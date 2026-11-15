#pragma once
#include <QMainWindow>
#include <QString>

#include <opencv2/core.hpp>

#include "core/Types.hpp"

class CarveWorker;
class DropImageView;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSlider;
class QSpinBox;
class QThread;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void loadImage(const QString& path);

private slots:
    void onOpen();
    void onRun();
    void onCancel();
    void onProgress(int done, int total);
    void onFinished(sc::CarveResult result);
    void onFailed(const QString& message);

private:
    void buildUi();
    void setRunning(bool running);

    DropImageView* view_ = nullptr;
    QPushButton* openBtn_ = nullptr;
    QPushButton* runBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QSlider* widthSlider_ = nullptr;
    QSlider* heightSlider_ = nullptr;
    QSpinBox* widthSpin_ = nullptr;
    QSpinBox* heightSpin_ = nullptr;
    QComboBox* algoBox_ = nullptr;
    QComboBox* energyBox_ = nullptr;
    QProgressBar* bar_ = nullptr;
    QLabel* infoLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    cv::Mat original_;
    CarveWorker* worker_ = nullptr;
    QThread* thread_ = nullptr;
    QString runAlgorithmName_;
    sc::Algorithm runAlgorithm_ = sc::Algorithm::DP;
    sc::EnergyMode runEnergy_ = sc::EnergyMode::Sobel;
};
