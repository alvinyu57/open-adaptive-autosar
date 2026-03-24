find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

set(OPEN_AA_TEST_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

add_subdirectory(unit)
add_subdirectory(smoke)
