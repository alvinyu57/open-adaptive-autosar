add_executable(openaa_core_smoke_test
    core_smoke_test.cpp
)

set_target_properties(openaa_core_smoke_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_TEST_RUNTIME_OUTPUT_DIRECTORY}"
)

target_link_libraries(openaa_core_smoke_test
    PRIVATE
        ara_com_lib
        ara_core_lib
        ara_exec_lib
)

target_compile_features(openaa_core_smoke_test PRIVATE cxx_std_20)

add_test(
    NAME openaa_core_smoke_test
    COMMAND openaa_core_smoke_test
)

set_tests_properties(
    openaa_core_smoke_test
    PROPERTIES
        LABELS "smoke"
)
