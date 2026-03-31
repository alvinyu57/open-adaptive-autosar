add_executable(openaa_app_manifest_demo
    main.cpp
)

set_target_properties(openaa_app_manifest_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_APP_OUTPUT_DIRECTORY}/02_app_manifest"
)

target_link_libraries(openaa_app_manifest_demo
    PRIVATE
        libara_exec
)

configure_file(
    manifests/application_manifest.json
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/application_manifest.json
    COPYONLY
)

configure_file(
    manifests/execution_manifest.json
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/execution_manifest.json
    COPYONLY
)

configure_file(
    manifests/application_manifest.arxml
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/application_manifest.arxml
    COPYONLY
)

configure_file(
    manifests/execution_manifest.arxml
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/execution_manifest.arxml
    COPYONLY
)
