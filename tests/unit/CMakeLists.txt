find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(openaa_core_unit_test
    core_application_test.cpp
    com_error_domain_test.cpp
    com_runtime_test.cpp
    com_service_types_test.cpp
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
        ara_com_lib
        ara_core_lib
        ara_exec_lib
        ara_log_lib
)

gtest_discover_tests(
    openaa_core_unit_test
    DISCOVERY_MODE PRE_TEST
    PROPERTIES
        LABELS "unit"
)
