#include "gui/MainWindow.hpp"

#include <algorithm>

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>

#include "gui/CarveWorker.hpp"
#include "gui/DropImageView.hpp"
#include "gui/MatQtConvert.hpp"
#include "gui/ResultDialog.hpp"

namespace {
// A slider and a spin box kept in sync.
void linkSliderAndSpin(QSlider* slider, QSpinBox* spin) {
    QObject::connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    QObject::connect(spin, &QSpinBox::valueChanged, slider, &QSlider::setValue);
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Seam Carving");
    buildUi();
}

MainWindow::~MainWindow() {
    // Stop a running carve before the window goes away.
    if (thread_) {
        if (worker_) worker_->cancel();
        thread_->quit();
        thread_->wait();
    }
}

void MainWindow::buildUi() {
    view_ = new DropImageView;
    connect(view_, &DropImageView::imageDropped, this, &MainWindow::loadImage);

    openBtn_ = new QPushButton("Open image...");
    infoLabel_ = new QLabel("No image loaded");

    widthSlider_ = new QSlider(Qt::Horizontal);
    heightSlider_ = new QSlider(Qt::Horizontal);
    widthSpin_ = new QSpinBox;
    heightSpin_ = new QSpinBox;
    for (QSpinBox* s : {widthSpin_, heightSpin_}) s->setRange(1, 1);
    for (QSlider* s : {widthSlider_, heightSlider_}) s->setRange(1, 1);
    linkSliderAndSpin(widthSlider_, widthSpin_);
    linkSliderAndSpin(heightSlider_, heightSpin_);

    auto* wRow = new QHBoxLayout;
    wRow->addWidget(widthSlider_, 1);
    wRow->addWidget(widthSpin_);
    auto* hRow = new QHBoxLayout;
    hRow->addWidget(heightSlider_, 1);
    hRow->addWidget(heightSpin_);

    algoBox_ = new QComboBox;
    algoBox_->addItem("Dynamic programming", static_cast<int>(sc::Algorithm::DP));
    algoBox_->addItem("Greedy", static_cast<int>(sc::Algorithm::Greedy));
    algoBox_->addItem("Graph cut (min-cut)", static_cast<int>(sc::Algorithm::GraphCut));

    energyBox_ = new QComboBox;
    energyBox_->addItem("Sobel (|gx| + |gy|)", static_cast<int>(sc::EnergyMode::Sobel));
    energyBox_->addItem("Gradient (sqrt)", static_cast<int>(sc::EnergyMode::Gradient));

    auto* form = new QFormLayout;
    form->addRow("Target width", wRow);
    form->addRow("Target height", hRow);
    form->addRow("Algorithm", algoBox_);
    form->addRow("Energy", energyBox_);
    auto* group = new QGroupBox("Resize");
    group->setLayout(form);

    runBtn_ = new QPushButton("Run");
    cancelBtn_ = new QPushButton("Cancel");
    runBtn_->setEnabled(false);
    cancelBtn_->setEnabled(false);
    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(runBtn_);
    btnRow->addWidget(cancelBtn_);

    bar_ = new QProgressBar;
    bar_->setRange(0, 1);
    bar_->setValue(0);
    statusLabel_ = new QLabel("Drop an image to begin.");
    statusLabel_->setWordWrap(true);

    auto* side = new QVBoxLayout;
    side->addWidget(openBtn_);
    side->addWidget(infoLabel_);
    side->addWidget(group);
    side->addLayout(btnRow);
    side->addWidget(bar_);
    side->addWidget(statusLabel_);
    side->addStretch(1);

    auto* sideWidget = new QWidget;
    sideWidget->setLayout(side);
    sideWidget->setFixedWidth(300);

    auto* root = new QHBoxLayout;
    root->addWidget(view_, 1);
    root->addWidget(sideWidget);
    auto* central = new QWidget;
    central->setLayout(root);
    setCentralWidget(central);
    resize(980, 600);

    connect(openBtn_, &QPushButton::clicked, this, &MainWindow::onOpen);
    connect(runBtn_, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(cancelBtn_, &QPushButton::clicked, this, &MainWindow::onCancel);
}

void MainWindow::onOpen() {
    const QString path =
        QFileDialog::getOpenFileName(this, "Open image", QString(), "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty()) loadImage(path);
}

void MainWindow::loadImage(const QString& path) {
    if (thread_) {
        statusLabel_->setText("Wait for the current run to finish (or cancel it) before loading a new image.");
        return;
    }
    cv::Mat img = loadImageFile(path);
    if (img.empty()) {
        QMessageBox::warning(this, "Cannot open image", "Could not read \"" + path + "\" as an image.");
        return;
    }
    original_ = img;
    view_->setImage(matToQImage(original_));

    for (QSpinBox* s : {widthSpin_, heightSpin_}) s->blockSignals(true);
    widthSpin_->setRange(1, original_.cols);
    heightSpin_->setRange(1, original_.rows);
    for (QSpinBox* s : {widthSpin_, heightSpin_}) s->blockSignals(false);
    widthSlider_->setRange(1, original_.cols);
    heightSlider_->setRange(1, original_.rows);
    widthSlider_->setValue(std::max(1, original_.cols * 4 / 5));  // default: 80% width
    heightSlider_->setValue(original_.rows);

    infoLabel_->setText(
        QString("%1 (%2 x %3)").arg(QFileInfo(path).fileName()).arg(original_.cols).arg(original_.rows));
    statusLabel_->setText("Choose a target size and press Run.");
    bar_->setValue(0);
    runBtn_->setEnabled(true);
}

void MainWindow::onRun() {
    if (original_.empty() || thread_) return;

    const int targetW = widthSpin_->value();
    const int targetH = heightSpin_->value();
    if (targetW == original_.cols && targetH == original_.rows) {
        statusLabel_->setText("Target size equals the original, nothing to do.");
        return;
    }

    const auto algorithm = static_cast<sc::Algorithm>(algoBox_->currentData().toInt());
    const auto energy = static_cast<sc::EnergyMode>(energyBox_->currentData().toInt());
    runAlgorithmName_ = algoBox_->currentText();
    runAlgorithm_ = algorithm;
    runEnergy_ = energy;

    thread_ = new QThread(this);
    worker_ = new CarveWorker(original_, targetW, targetH, algorithm, energy);
    worker_->moveToThread(thread_);

    connect(thread_, &QThread::started, worker_, &CarveWorker::run);
    connect(worker_, &CarveWorker::progress, this, &MainWindow::onProgress);
    connect(worker_, &CarveWorker::finished, this, &MainWindow::onFinished);
    connect(worker_, &CarveWorker::failed, this, &MainWindow::onFailed);
    // Direct connections: QThread::quit() is thread-safe, and this lets the destructor's
    // thread_->wait() return even though the GUI event loop is blocked.
    connect(worker_, &CarveWorker::finished, thread_, &QThread::quit, Qt::DirectConnection);
    connect(worker_, &CarveWorker::failed, thread_, &QThread::quit, Qt::DirectConnection);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(thread_, &QThread::finished, thread_, &QObject::deleteLater);

    bar_->setRange(0, std::max(1, (original_.cols - 1) + (original_.rows - 1)));
    bar_->setValue(0);
    statusLabel_->setText("Running...");
    setRunning(true);
    thread_->start();
}

void MainWindow::onCancel() {
    if (worker_) {
        worker_->cancel();
        statusLabel_->setText("Cancelling...");
    }
}

void MainWindow::onProgress(int done, int total) {
    bar_->setRange(0, total);
    bar_->setValue(done);
}

void MainWindow::onFinished(sc::CarveResult result) {
    worker_ = nullptr;  // both objects delete themselves once the thread has finished
    thread_ = nullptr;
    setRunning(false);

    if (result.cancelled) {
        statusLabel_->setText(QString("Cancelled after %1 seams.").arg(result.seamsRemoved));
        return;
    }
    statusLabel_->setText(QString("%1 finished in %2 ms (%3 seams).")
                              .arg(runAlgorithmName_)
                              .arg(result.elapsedMs, 0, 'f', 1)
                              .arg(result.seamsRemoved));

    // Non-modal and resizable: dragging its edges re-carves the original live.
    auto* dialog = new ResultDialog(runAlgorithmName_, runAlgorithm_, runEnergy_, original_, result, this);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::onFailed(const QString& message) {
    worker_ = nullptr;
    thread_ = nullptr;
    setRunning(false);
    statusLabel_->setText("Failed: " + message);
    QMessageBox::warning(this, "Seam carving failed", message);
}

void MainWindow::setRunning(bool running) {
    runBtn_->setEnabled(!running && !original_.empty());
    cancelBtn_->setEnabled(running);
    openBtn_->setEnabled(!running);
    for (QWidget* w : std::initializer_list<QWidget*>{widthSlider_, heightSlider_, widthSpin_, heightSpin_,
                                                      algoBox_, energyBox_})
        w->setEnabled(!running);
}
