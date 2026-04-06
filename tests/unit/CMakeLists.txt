find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(openaa_core_unit_test
    core_application_test.cpp
    execution_manager_manifest_test.cpp
    core_error_domain_test.cpp
    logger_test.cpp
    initialization_test.cpp
    abort_test.cpp
)

set_target_properties(openaa_core_unit_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_TEST_RUNTIME_OUTPUT_DIRECTORY}"
)

target_link_libraries(openaa_core_unit_test
    PRIVATE
        GTest::gtest_main
        ara_core_lib
        ara_exec_lib
        ara_log_lib
)

target_compile_features(openaa_core_unit_test PRIVATE cxx_std_20)

gtest_discover_tests(
    openaa_core_unit_test
    PROPERTIES
        LABELS "unit"
)
