#include "gui/CarveWorker.hpp"

#include <exception>
#include <utility>

CarveWorker::CarveWorker(cv::Mat source, int targetW, int targetH, sc::Algorithm algorithm, sc::EnergyMode energy, QObject* parent)
    : QObject(parent), source_(std::move(source)), targetW_(targetW), targetH_(targetH), algorithm_(algorithm), energy_(energy) 
    { /*Empty by design*/ }

void CarveWorker::run() {
    try {
        auto finder = sc::makeFinder(algorithm_);
        sc::CarveOptions options;
        options.energy = energy_;

        sc::CarveResult result = sc::carve(
            source_, targetW_, targetH_, *finder, options, [this](int done, int total, const sc::Seam&) {
                emit progress(done, total);
                return !cancel_.load();
            });
        emit finished(result);
    } catch (const std::exception& e) {
        emit failed(QString::fromUtf8(e.what()));
    }
}
