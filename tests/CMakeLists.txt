cmake_minimum_required(VERSION 3.27)
project("${CMAKE_PROJECT_NAME}_tests" CXX)

find_package(Catch2 3 REQUIRED)

set(TESTS_SRC src/test_ring.cc src/test_enum_flags.cc)

set(ASan_FLAGS -fsanitize=address -fno-omit-frame-pointer)
set(MSan_FLAGS -fsanitize=memory -fno-omit-frame-pointer)
set(UBSan_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
set(TSan_FLAGS -fsanitize=thread -fno-omit-frame-pointer)

if(NOT (MSVC))
  set(SANITIZERS ASan UBSan TSan)
endif()

foreach(san IN LISTS SANITIZERS)
  set(TARGET utests-${san})
  set(SAN_FLAGS ${${san}_FLAGS})

  add_executable(${TARGET})

  target_sources(${TARGET} PRIVATE ${TESTS_SRC})
  target_link_libraries(${TARGET} PRIVATE Catch2::Catch2WithMain)

  target_compile_options(${TARGET} PRIVATE ${SAN_FLAGS})
  target_link_libraries(${TARGET} PRIVATE ${SAN_FLAGS})

  add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

if(NOT (SANITIZERS))
  add_executable(utests)

  target_sources(utests PRIVATE ${TESTS_SRC})
  target_link_libraries(utests PRIVATE Catch2::Catch2WithMain)

  add_test(NAME utests COMMAND utests)
endif()
