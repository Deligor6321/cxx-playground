cmake_minimum_required(VERSION 3.27)
project(bench CXX)

find_package(benchmark REQUIRED)

add_executable(${PROJECT_NAME} src/bench_concurrent.cc)
target_link_libraries(${PROJECT_NAME} benchmark::benchmark)