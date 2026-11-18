#include "gui/CarveWorker.hpp"

#include <chrono>
#include <exception>
#include <utility>

CarveWorker::CarveWorker(cv::Mat source, int targetW, int targetH, sc::Algorithm algorithm, sc::EnergyMode energy, QObject* parent)
    : QObject(parent), source_(std::move(source)), targetW_(targetW), targetH_(targetH), algorithm_(algorithm), energy_(energy) 
    { /*Empty by design*/ }

void CarveWorker::run() {
    try {
        const auto t0 = std::chrono::steady_clock::now();
        auto finder = sc::makeFinder(algorithm_);
        sc::CarveOptions options;
        options.energy = energy_;

        bool ok = finder->init(source_, 1, 1, options, [this](int done, int total, const sc::Seam&) {
            emit progress(done, total);
            return !cancel_.load();
        });

        sc::CarveResult result;
        result.image = finder->reconstruct(source_, targetW_, targetH_);
        result.vMap = finder->vMap();
        result.hMap = finder->hMap();
        result.removedOrder = finder->buildRemovedOrder(source_, targetW_, targetH_);
        result.seamsRemoved = (source_.cols - targetW_) + (source_.rows - targetH_);
        result.cancelled = !ok;
        result.elapsedMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

        emit finished(result);
    } catch (const std::exception& e) {
        emit failed(QString::fromUtf8(e.what()));
    }
}
