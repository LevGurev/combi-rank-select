cmake_minimum_required(VERSION 3.14)
project(combi_rank_select LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# По умолчанию собираем с оптимизацией: иначе замеры скорости бессмысленны.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Тип сборки" FORCE)
endif()

# Библиотека — только заголовочные файлы в include/combi.
add_library(combi INTERFACE)
target_include_directories(combi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  # /utf-8 — исходники в UTF-8 (русские комментарии и строки)
  target_compile_options(combi INTERFACE /W4 /utf-8 /permissive-)
else()
  target_compile_options(combi INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(tests
  tests/test_main.cpp
  tests/test_biguint.cpp
  tests/test_permutations.cpp
  tests/test_combinations.cpp)
target_link_libraries(tests PRIVATE combi)

add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE combi)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE combi)

enable_testing()
add_test(NAME unit_tests COMMAND tests)
add_test(NAME demo_runs COMMAND demo)
add_test(NAME bench_quick COMMAND bench --quick)
