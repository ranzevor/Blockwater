if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The RDR2 plugin requires Windows x64 and the MSVC ABI")
endif()

set(scripthook_lib "${CMAKE_CURRENT_BINARY_DIR}/ScriptHookRDR2.lib")
add_custom_command(
    OUTPUT "${scripthook_lib}"
    COMMAND "${CMAKE_AR}" /nologo "/def:${CMAKE_CURRENT_SOURCE_DIR}/scripthook.def"
            "/out:${scripthook_lib}" /machine:x64
    DEPENDS scripthook.def
    VERBATIM)
add_custom_target(blockwater_scripthook_imports DEPENDS "${scripthook_lib}")

add_library(blockwater_rdr2 MODULE src/plugin.cpp)
set_target_properties(blockwater_rdr2 PROPERTIES
    PREFIX "" OUTPUT_NAME Blockwater SUFFIX ".asi"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugin/$<CONFIG>")
add_dependencies(blockwater_rdr2 blockwater_scripthook_imports)
target_link_libraries(blockwater_rdr2 PRIVATE blockwater_core "${scripthook_lib}")
target_compile_options(blockwater_rdr2 PRIVATE /W4 /WX /utf-8)

# The test runtime lives outside the plugin directory and is never installed.
set(harness_dir "${CMAKE_CURRENT_BINARY_DIR}/harness/$<CONFIG>")
add_library(blockwater_fake_scripthook SHARED tests/fake_scripthook.cpp)
target_compile_definitions(blockwater_fake_scripthook PRIVATE
    "BW_SCRIPTHOOK_API=__declspec(dllexport)")
target_compile_features(blockwater_fake_scripthook PRIVATE cxx_std_20)
target_compile_options(blockwater_fake_scripthook PRIVATE /W4 /WX /utf-8)
set_target_properties(blockwater_fake_scripthook PROPERTIES
    PREFIX "" OUTPUT_NAME ScriptHookRDR2
    RUNTIME_OUTPUT_DIRECTORY "${harness_dir}")

add_executable(blockwater_rdr2_loader tests/loader.cpp)
target_link_libraries(blockwater_rdr2_loader PRIVATE blockwater_core blockwater_fake_scripthook)
target_compile_options(blockwater_rdr2_loader PRIVATE /W4 /WX /utf-8)
set_target_properties(blockwater_rdr2_loader PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${harness_dir}")
foreach(scenario IN ITEMS story network0 network1 network2 network_with_null paused unavailable dead cutscene
        null_native disconnect restart reload corrupt duplicate early_unload)
    add_test(NAME "rdr2_${scenario}" COMMAND blockwater_rdr2_loader
        "$<TARGET_FILE:blockwater_rdr2>" "${scenario}")
    set_tests_properties("rdr2_${scenario}" PROPERTIES TIMEOUT 15
        RESOURCE_LOCK blockwater_rdr2_runtime)
endforeach()

add_executable(blockwater_rdr2_missing_runtime tests/missing_runtime.cpp)
target_compile_options(blockwater_rdr2_missing_runtime PRIVATE /W4 /WX /utf-8)
set_target_properties(blockwater_rdr2_missing_runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/probe/$<CONFIG>")
add_test(NAME rdr2_missing_runtime COMMAND blockwater_rdr2_missing_runtime
    "$<TARGET_FILE:blockwater_rdr2>")
set_tests_properties(rdr2_missing_runtime PROPERTIES TIMEOUT 15)

install(TARGETS blockwater_rdr2 RUNTIME DESTINATION . LIBRARY DESTINATION .)
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    "${PROJECT_SOURCE_DIR}/LICENSES/Garrys-Redemption.txt" DESTINATION licenses)

add_test(NAME rdr2_install_manifest COMMAND "${CMAKE_COMMAND}"
    "-DBUILD_DIR=${PROJECT_BINARY_DIR}" "-DCONFIG=$<CONFIG>"
    "-DDESTINATION=${CMAKE_CURRENT_BINARY_DIR}/package/$<CONFIG>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_package.cmake")
set_tests_properties(rdr2_install_manifest PROPERTIES TIMEOUT 20)
