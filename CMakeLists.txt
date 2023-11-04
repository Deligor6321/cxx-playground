cmake_minimum_required(VERSION 3.27)
project(cxx-playground CXX)

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
add_compile_options(
  -Wall
  -Wextra
  -Wshadow
  -Wnon-virtual-dtor
  -Wcast-align
  -Wunused
  -Woverloaded-virtual
  -Wconversion
  -Weffc++
  -pedantic)

add_subdirectory(benchmarks)
