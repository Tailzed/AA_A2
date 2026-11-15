#include "core/SeamFinder.hpp"

#include <stdexcept>

#include "core/DpSeamFinder.hpp"
#include "core/GraphCutSeamFinder.hpp"
#include "core/GreedySeamFinder.hpp"

namespace sc {

std::unique_ptr<ISeamFinder> makeFinder(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::DP: return std::make_unique<DpSeamFinder>();
        case Algorithm::Greedy: return std::make_unique<GreedySeamFinder>();
        case Algorithm::GraphCut: return std::make_unique<GraphCutSeamFinder>();
    }
    throw std::invalid_argument("makeFinder: unknown algorithm");
}

}  // namespace sc
