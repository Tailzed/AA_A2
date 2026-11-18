#include "gui/ResultDialog.hpp"

#include <algorithm>

#include <QCheckBox>
#include <QEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

#include <chrono>

#include "core/SeamFinder.hpp"
#include "core/SeamOps.hpp"
#include "gui/CarveWorker.hpp"
#include "gui/MatQtConvert.hpp"

ResultDialog::ResultDialog(const QString& algorithmName, sc::Algorithm algorithm, sc::EnergyMode energy,
                           const cv::Mat& original, const sc::CarveResult& result, QWidget* parent)
    : QDialog(parent),
      algorithmName_(algorithmName),
      algorithm_(algorithm),
      energy_(energy),
      original_(original),
      result_(result) {
    finder_ = sc::makeFinder(algorithm_);
    if (!result_.vMap.empty()) finder_->vMap() = result_.vMap;
    if (!result_.hMap.empty()) finder_->hMap() = result_.hMap;

    setAttribute(Qt::WA_DeleteOnClose);  // non-modal: opened with show(), never exec()
    setSizeGripEnabled(true);

    // The label's size IS the carve target, so it must never grow because of its pixmap.
    imageLabel_ = new QLabel;
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setMinimumSize(60, 60);
    imageLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel_->setStyleSheet("QLabel { background: #2b2b2b; }");
    imageLabel_->installEventFilter(this);

    infoLabel_ = new QLabel;
    infoLabel_->setWordWrap(true);
    statusLabel_ = new QLabel("Drag the window edges to resize (content-aware, shrink only).");
    statusLabel_->setWordWrap(true);
    bar_ = new QProgressBar;
    bar_->setRange(0, 1);
    bar_->setValue(0);
    bar_->setMaximumHeight(8);
    bar_->setTextVisible(false);

    seamBox_ = new QCheckBox("Show removed seams (drawn on the original)");
    stepSlider_ = new QSlider(Qt::Horizontal);
    stepSlider_->setEnabled(false);
    stepLabel_ = new QLabel;

    auto* saveBtn = new QPushButton("Save result...");
    auto* closeBtn = new QPushButton("Close");

    auto* stepRow = new QHBoxLayout;
    stepRow->addWidget(new QLabel("Seams shown:"));
    stepRow->addWidget(stepSlider_, 1);
    stepRow->addWidget(stepLabel_);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(saveBtn);
    buttons->addWidget(closeBtn);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(infoLabel_);
    layout->addWidget(imageLabel_, 1);
    layout->addWidget(bar_);
    layout->addWidget(statusLabel_);
    layout->addWidget(seamBox_);
    layout->addLayout(stepRow);
    layout->addLayout(buttons);

    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(150);  // carve once the user pauses, not on every pixel of the drag
    connect(debounce_, &QTimer::timeout, this, &ResultDialog::onResizeSettled);

    connect(seamBox_, &QCheckBox::toggled, stepSlider_, &QSlider::setEnabled);
    connect(seamBox_, &QCheckBox::toggled, this, &ResultDialog::refresh);
    connect(stepSlider_, &QSlider::valueChanged, this, &ResultDialog::refresh);
    connect(saveBtn, &QPushButton::clicked, this, &ResultDialog::onSave);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    int w = result.image.cols + 40;
    int h = result.image.rows + 230;
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        w = std::min(w, avail.width() - 80);
        h = std::min(h, avail.height() - 80);
    }
    resize(std::max(w, 520), std::max(h, 440));

    applyResult();
}

ResultDialog::~ResultDialog() {
    // Stop a running carve before the window goes away (waits for the current seam to finish).
    if (thread_) {
        if (worker_) worker_->cancel();
        thread_->quit();
        thread_->wait();
    }
}

void ResultDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    // Opening the window triggers resize events; only react to the user's resizing after that.
    QTimer::singleShot(400, this, [this] { ready_ = true; });
}

bool ResultDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == imageLabel_ && event->type() == QEvent::Resize) {
        if (ready_ && finder_ && finder_->hasMaps()) {
            const int targetW = std::clamp(imageLabel_->width(), 1, original_.cols);
            const int targetH = std::clamp(imageLabel_->height(), 1, original_.rows);
            if (targetW != result_.image.cols || targetH != result_.image.rows) {
                const auto t0 = std::chrono::steady_clock::now();
                result_.image = finder_->reconstruct(original_, targetW, targetH);
                result_.removedOrder = finder_->buildRemovedOrder(original_, targetW, targetH);
                result_.seamsRemoved = (original_.cols - targetW) + (original_.rows - targetH);
                result_.elapsedMs =
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                applyResult();
                return QDialog::eventFilter(watched, event);
            }
        }
        refresh();                           // immediate feedback: show the current result fitted to the label
        if (ready_) debounce_->start();      // the real carve follows once the resizing pauses
    }
    return QDialog::eventFilter(watched, event);
}

void ResultDialog::onResizeSettled() {
    const QSize target(std::clamp(imageLabel_->width(), 1, original_.cols),
                       std::clamp(imageLabel_->height(), 1, original_.rows));
    requestCarve(target);
}

void ResultDialog::requestCarve(const QSize& target) {
    const QSize current(result_.image.cols, result_.image.rows);
    if (thread_) {
        if (target == runningTarget_) {  // already carving to exactly this size
            pending_.reset();
            return;
        }
        pending_ = target;
        if (worker_) worker_->cancel();  // the newer size wins; startNext() runs when the thread has ended
        return;
    }
    if (target == current) {
        pending_.reset();
        return;
    }
    pending_ = target;
    startNext();
}

void ResultDialog::startNext() {
    if (!pending_ || thread_) return;
    const QSize target = *pending_;
    pending_.reset();
    if (target == QSize(result_.image.cols, result_.image.rows)) return;

    runningTarget_ = target;
    thread_ = new QThread(this);
    worker_ = new CarveWorker(original_, target.width(), target.height(), algorithm_, energy_);
    worker_->moveToThread(thread_);

    connect(thread_, &QThread::started, worker_, &CarveWorker::run);
    connect(worker_, &CarveWorker::progress, this, &ResultDialog::onProgress);
    connect(worker_, &CarveWorker::finished, this, &ResultDialog::onFinished);
    connect(worker_, &CarveWorker::failed, this, &ResultDialog::onFailed);
    // Direct: quit() is thread-safe and works even while the GUI thread is busy.
    connect(worker_, &CarveWorker::finished, thread_, &QThread::quit, Qt::DirectConnection);
    connect(worker_, &CarveWorker::failed, thread_, &QThread::quit, Qt::DirectConnection);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(thread_, &QThread::finished, thread_, &QObject::deleteLater);
    connect(thread_, &QThread::finished, this, &ResultDialog::onThreadFinished);

    statusLabel_->setText(QString("Carving to %1 x %2 ...").arg(target.width()).arg(target.height()));
    bar_->setRange(0, 0);  // busy indicator until the first seam reports back
    thread_->start();
}

void ResultDialog::onProgress(int done, int total) {
    bar_->setRange(0, total);
    bar_->setValue(done);
}

void ResultDialog::onFinished(sc::CarveResult result) {
    worker_ = nullptr;  // it has reported back; it is deleted when the thread ends
    if (result.cancelled) return;  // superseded by a newer size, or the dialog is closing
    result_ = std::move(result);
    applyResult();
    statusLabel_->setText(QString("%1 x %2 in %3 ms.  Drag the window edges to resize again.")
                              .arg(result_.image.cols)
                              .arg(result_.image.rows)
                              .arg(result_.elapsedMs, 0, 'f', 0));
}

void ResultDialog::onFailed(const QString& message) {
    worker_ = nullptr;
    statusLabel_->setText("Failed: " + message);
}

void ResultDialog::onThreadFinished() {
    thread_ = nullptr;  // QThread and the worker delete themselves
    worker_ = nullptr;
    bar_->setRange(0, 1);
    bar_->setValue(0);
    startNext();  // a newer size may have been requested while this one was running
}

void ResultDialog::applyResult() {
    setWindowTitle(QString("%1 | %2 ms | %3x%4")
                       .arg(algorithmName_)
                       .arg(result_.elapsedMs, 0, 'f', 0)
                       .arg(result_.image.cols)
                       .arg(result_.image.rows));
    infoLabel_->setText(QString("%1 | original %2x%3 -> %4x%5 | %6 seams removed | %7 ms")
                            .arg(algorithmName_)
                            .arg(original_.cols)
                            .arg(original_.rows)
                            .arg(result_.image.cols)
                            .arg(result_.image.rows)
                            .arg(result_.seamsRemoved)
                            .arg(result_.elapsedMs, 0, 'f', 1));

    stepSlider_->blockSignals(true);
    stepSlider_->setRange(0, std::max(1, result_.seamsRemoved));
    stepSlider_->setValue(result_.seamsRemoved);
    stepSlider_->blockSignals(false);
    refresh();
}

void ResultDialog::refresh() {
    QImage img;
    if (seamBox_->isChecked()) {
        if (!result_.removedOrder.empty() && result_.removedOrder.size() == original_.size()) {
            img = matToQImage(sc::overlaySeams(original_, result_.removedOrder, stepSlider_->value()));
        } else {
            img = matToQImage(result_.image);
        }
        stepLabel_->setText(QString("%1 / %2").arg(stepSlider_->value()).arg(result_.seamsRemoved));
    } else {
        img = matToQImage(result_.image);
        stepLabel_->setText(QString("- / %1").arg(result_.seamsRemoved));
    }
    QPixmap pm = QPixmap::fromImage(img);
    // Shown 1:1 whenever it fits; only scaled down (for display) while the window is smaller
    // than the current result, i.e. until the carve for the new size has finished.
    if (pm.width() > imageLabel_->width() || pm.height() > imageLabel_->height())
        pm = pm.scaled(imageLabel_->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
    imageLabel_->setPixmap(pm);
}

void ResultDialog::onSave() {
    const QString path = QFileDialog::getSaveFileName(this, "Save result", "carved.png",
                                                      "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");
    if (path.isEmpty()) return;
    if (!matToQImage(result_.image).save(path))
        QMessageBox::warning(this, "Save failed", "Could not write " + path);
}
