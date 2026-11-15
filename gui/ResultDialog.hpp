#pragma once
#include <optional>

#include <QDialog>
#include <QSize>
#include <QString>

#include <opencv2/core.hpp>

#include "core/Types.hpp"

class CarveWorker;
class QCheckBox;
class QEvent;
class QLabel;
class QProgressBar;
class QShowEvent;
class QSlider;
class QThread;
class QTimer;

// Non-modal, resizable result window. Drag its edges and the image is seam-carved from the ORIGINAL to
// the new size (shrink only: the target is clamped to the original size). Resizing is debounced; a
// running carve is cancelled when a newer size is requested. One window per run, deletes itself on close.
class ResultDialog : public QDialog {
    Q_OBJECT
public:
    ResultDialog(const QString& algorithmName, sc::Algorithm algorithm, sc::EnergyMode energy,
                 const cv::Mat& original, const sc::CarveResult& result, QWidget* parent = nullptr);
    ~ResultDialog() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void refresh();
    void onSave();
    void onResizeSettled();
    void onProgress(int done, int total);
    void onFinished(sc::CarveResult result);
    void onFailed(const QString& message);
    void onThreadFinished();

private:
    void requestCarve(const QSize& target);
    void startNext();
    void applyResult();

    QString algorithmName_;
    sc::Algorithm algorithm_;
    sc::EnergyMode energy_;
    cv::Mat original_;
    sc::CarveResult result_;

    QLabel* imageLabel_ = nullptr;
    QLabel* infoLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* stepLabel_ = nullptr;
    QProgressBar* bar_ = nullptr;
    QCheckBox* seamBox_ = nullptr;
    QSlider* stepSlider_ = nullptr;
    QTimer* debounce_ = nullptr;

    bool ready_ = false;                // ignore the resize events caused by the window opening
    std::optional<QSize> pending_;      // newest requested size not started yet
    QSize runningTarget_;
    QThread* thread_ = nullptr;         // kept until QThread::finished, so it is never destroyed while running
    CarveWorker* worker_ = nullptr;     // nulled as soon as the worker has reported back
};
