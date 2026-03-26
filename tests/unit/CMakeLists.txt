add_executable(openaa_core_unit_test
    core_application_test.cpp
)

set_target_properties(openaa_core_unit_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_TEST_RUNTIME_OUTPUT_DIRECTORY}"
)

target_link_libraries(openaa_core_unit_test
    PRIVATE
        GTest::gtest_main
        openaa_platform_core
)

target_compile_features(openaa_core_unit_test PRIVATE cxx_std_20)

gtest_discover_tests(
    openaa_core_unit_test
    PROPERTIES
        LABELS "unit"
)
