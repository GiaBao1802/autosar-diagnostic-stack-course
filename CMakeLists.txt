cmake_minimum_required(VERSION 3.16)
project(autosar_diagnostic_stack_course LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Werror)
endif()

enable_testing()

add_library(diag_server STATIC src/diag_server.cpp)
target_include_directories(diag_server PUBLIC include)

add_executable(diag_demo app/diag_demo.cpp)
target_link_libraries(diag_demo PRIVATE diag_server)

add_executable(diag_tests tests/test_diag.cpp)
target_link_libraries(diag_tests PRIVATE diag_server)
add_test(NAME diag_tests COMMAND diag_tests)
