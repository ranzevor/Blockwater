cmake_minimum_required(VERSION 3.20)
project(blockwater VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(blockwater_core STATIC
    src/mapping.cpp
    src/host_adapter.cpp
    src/host_session.cpp
    src/world_stream.cpp
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

add_executable(blockwater_session_tests tests/test_host_session.cpp)
target_link_libraries(blockwater_session_tests PRIVATE blockwater_core)
if(MSVC)
    target_compile_options(blockwater_session_tests PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(blockwater_session_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME blockwater_host_session COMMAND blockwater_session_tests)
set_tests_properties(blockwater_host_session PROPERTIES TIMEOUT 20)

add_executable(blockwater_world_tests tests/test_world_stream.cpp)
target_link_libraries(blockwater_world_tests PRIVATE blockwater_core)
if(MSVC)
    target_compile_options(blockwater_world_tests PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(blockwater_world_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME blockwater_world_stream COMMAND blockwater_world_tests)
set_tests_properties(blockwater_world_stream PROPERTIES TIMEOUT 20)

option(BLOCKWATER_BUILD_RDR2 "Build the Windows x64 ScriptHookRDR2 plugin" ${WIN32})
if(BLOCKWATER_BUILD_RDR2)
    add_subdirectory(rdr2)
endif()
