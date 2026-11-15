#pragma once
#include <atomic>

#include <QMetaType>
#include <QObject>
#include <QString>

#include <opencv2/core.hpp>

#include "core/SeamCarver.hpp"

Q_DECLARE_METATYPE(sc::CarveResult)

// Runs sc::carve() off the GUI thread. The only place where Qt and the core meet.
// Usage: moveToThread(thread); connect(thread, &QThread::started, worker, &CarveWorker::run);
// All inputs are set in the constructor, so no cv::Mat has to travel through a queued slot call.
class CarveWorker : public QObject {
    Q_OBJECT
public:
    CarveWorker(cv::Mat source, int targetW, int targetH, sc::Algorithm algorithm,
                sc::EnergyMode energy, QObject* parent = nullptr);

    void cancel() { cancel_.store(true); }  // thread-safe

public slots:
    void run();

signals:
    void progress(int done, int total);
    void finished(sc::CarveResult result);  // also emitted when cancelled (result.cancelled == true)
    void failed(const QString& message);

private:
    cv::Mat source_;
    int targetW_;
    int targetH_;
    sc::Algorithm algorithm_;
    sc::EnergyMode energy_;
    std::atomic<bool> cancel_{false};
};
