find_package(Catch2 3 CONFIG REQUIRED)

add_executable(seamcore_tests
  TestUtils.hpp
  test_energy.cpp
  test_ops.cpp
  test_carver.cpp
  test_contract.cpp
  test_optimality.cpp
)
target_link_libraries(seamcore_tests PRIVATE seamcore Catch2::Catch2WithMain)

include(Catch)
# PRE_TEST: tests are listed when ctest runs, not while building.
catch_discover_tests(seamcore_tests DISCOVERY_MODE PRE_TEST)
