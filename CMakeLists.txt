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

find_package(Microsoft.GSL CONFIG REQUIRED)

add_library(dlgr STATIC)
target_sources(dlgr PUBLIC include/dlgr/enum_flags.h include/dlgr/ring.h src/dummy.cc)
target_include_directories(dlgr PUBLIC include)
target_link_libraries(dlgr PUBLIC Microsoft.GSL::GSL)

add_subdirectory(benchmarks)
add_subdirectory(tests)
