# Core library: OpenCV only, no Qt. The tests link this directly.
add_library(seamcore STATIC
  Types.hpp
  Energy.hpp            Energy.cpp
  SeamOps.hpp           SeamOps.cpp
  SeamFinder.hpp        SeamFinder.cpp
  SeamCarver.hpp        SeamCarver.cpp
  DpSeamFinder.hpp      DpSeamFinder.cpp
  GreedySeamFinder.hpp  GreedySeamFinder.cpp
  GraphCutSeamFinder.hpp GraphCutSeamFinder.cpp
)

# Headers are included as "core/Xxx.hpp", "gui/Xxx.hpp", "tests/Xxx.hpp" from the repo root.
target_include_directories(seamcore PUBLIC ${PROJECT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seamcore PUBLIC ${OpenCV_LIBS})
