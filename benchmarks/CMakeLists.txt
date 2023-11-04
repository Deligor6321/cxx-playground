cmake_minimum_required(VERSION 3.27)
project("${CMAKE_PROJECT_NAME}_benchmarks" CXX)

find_package(benchmark REQUIRED)

add_executable(benchmarks)
target_sources(benchmarks PRIVATE src/bm_concurrent.cc src/main.cc)

target_link_libraries(benchmarks benchmark::benchmark)
