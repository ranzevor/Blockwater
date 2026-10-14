cmake_minimum_required(VERSION 3.20)
project(blockwater VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(blockwater_core STATIC
    src/mapping.cpp
    src/host_adapter.cpp
    src/compositor.cpp
    src/i18n.cpp)
target_include_directories(blockwater_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(blockwater_core PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(blockwater_core PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(blockwater_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(blockwater_core PUBLIC rt)
endif()

add_executable(blockwater_fakes examples/fake_pair.cpp)
target_link_libraries(blockwater_fakes PRIVATE blockwater_core)

enable_testing()
add_executable(blockwater_tests
    tests/test_main.cpp
    tests/test_wire.cpp
    tests/test_ipc.cpp
    tests/test_host.cpp
    tests/test_compositor.cpp
    tests/test_i18n.cpp)
target_link_libraries(blockwater_tests PRIVATE blockwater_core)
if(MSVC)
    target_compile_options(blockwater_tests PRIVATE /utf-8)
endif()
target_compile_definitions(blockwater_tests PRIVATE BLOCKWATER_LOCALES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/locales")
add_test(NAME blockwater_tests COMMAND blockwater_tests)

add_test(NAME blockwater_fake_pair COMMAND blockwater_fakes)
