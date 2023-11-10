cmake_minimum_required(VERSION 3.27)
project(cxx-playground CXX)

set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

if(MSVC)
  add_compile_options(/W4 /permissive /w14640)
else()
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
endif()

enable_testing()

include_directories(include)

add_subdirectory(benchmarks)
add_subdirectory(tests)
