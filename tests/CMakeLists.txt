cmake_minimum_required(VERSION 3.27)
project("${CMAKE_PROJECT_NAME}_tests" CXX)

find_package(Catch2 3 REQUIRED)

add_executable(tests)
target_sources(tests PRIVATE src/test_ring.cc src/test_enum_flags.cc)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME tests COMMAND tests)
