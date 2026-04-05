add_executable(openaa_example_hello_world_runner
    main.cpp
)

set_target_properties(openaa_example_hello_world_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_APP_OUTPUT_DIRECTORY}/01_hello_world"
    BUILD_RPATH "$ORIGIN/../../lib"
    INSTALL_RPATH "$ORIGIN/../../lib"
)

target_include_directories(openaa_example_hello_world_runner
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(openaa_example_hello_world_runner
    PRIVATE
        ara_exec_lib
)

target_compile_features(openaa_example_hello_world_runner PRIVATE cxx_std_20)

add_custom_command(
    TARGET  openaa_example_hello_world_runner PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/verify_arxml.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
)

add_custom_command(
    TARGET  openaa_example_hello_world_runner PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/arxml2json.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${CMAKE_BINARY_DIR}/apps/01_hello_world/manifests/app_execution_manifest.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
)

# [WORKAROUND] before manifest are correctly processed
##########################################################
add_custom_command(
    TARGET  openaa_example_hello_world_runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.json
            ${OPEN_AA_APP_OUTPUT_DIRECTORY}/01_hello_world/manifests/app_execution_manifest.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.json
)
##########################################################