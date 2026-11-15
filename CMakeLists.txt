cmake_minimum_required(VERSION 3.21)
project(seamcarve LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8 /W4)   # /utf-8: source files are UTF-8
else()
  add_compile_options(-Wall -Wextra)
endif()

option(BUILD_GUI   "Build the Qt GUI (seamcarve_app)" ON)
option(BUILD_TESTS "Build the Catch2 tests (seamcore_tests)" ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

add_subdirectory(core)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_GUI)
  add_subdirectory(gui)
endif()
