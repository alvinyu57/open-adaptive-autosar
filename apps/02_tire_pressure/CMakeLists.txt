set(TP_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/02_tire_pressure")

file(MAKE_DIRECTORY 
    "${TP_OUTPUT_DIRECTORY}/manifests"
    "${TP_OUTPUT_DIRECTORY}/data"
)

add_executable(openaa_tire_pressure_provider
    main_tp_provider.cpp
)

set_target_properties(openaa_tire_pressure_provider PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TP_OUTPUT_DIRECTORY}"
    BUILD_RPATH "$ORIGIN/../../lib"
    INSTALL_RPATH "$ORIGIN/../../lib"
)

target_include_directories(openaa_tire_pressure_provider
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/tire_pressure_data.json
     DESTINATION ${TP_OUTPUT_DIRECTORY}/data)

target_link_libraries(openaa_tire_pressure_provider
    PRIVATE
        ara_exec_lib
        ara_com_lib
        ara_log_lib
)

target_compile_definitions(openaa_tire_pressure_provider PRIVATE
    OPEN_AA_TIRE_PRESSURE_DATA_FILE="${CMAKE_CURRENT_SOURCE_DIR}/data/tire_pressure_data.json"
    OPEN_AA_TIRE_PRESSURE_SERVICE_MANIFEST="${TP_OUTPUT_DIRECTORY}/manifests/service_instance_manifest.json"
)

add_custom_command(
    OUTPUT ${TP_OUTPUT_DIRECTORY}/manifests/app_execution_manifest.json
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/verify_arxml.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/arxml2json.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${TP_OUTPUT_DIRECTORY}/manifests/app_execution_manifest.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/manifests/app_execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
)

add_custom_command(
    OUTPUT ${TP_OUTPUT_DIRECTORY}/manifests/service_instance_manifest.json
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/verify_arxml.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/service_instance_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/arxml2json.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manifests/service_instance_manifest.arxml
            ${TP_OUTPUT_DIRECTORY}/manifests/service_instance_manifest.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/manifests/service_instance_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
)

add_custom_target(openaa_tire_pressure_manifest ALL
    DEPENDS
        ${TP_OUTPUT_DIRECTORY}/manifests/app_execution_manifest.json
        ${TP_OUTPUT_DIRECTORY}/manifests/service_instance_manifest.json
)

set_property(GLOBAL APPEND PROPERTY OPEN_AA_APP_EXECUTION_MANIFEST_JSONS
    "${TP_OUTPUT_DIRECTORY}/manifests/app_execution_manifest.json"
)

set_property(GLOBAL APPEND PROPERTY OPEN_AA_APP_TARGETS
    openaa_tire_pressure_provider
    openaa_tire_pressure_consumer
    openaa_tire_pressure_manifest
)

add_executable(openaa_tire_pressure_consumer
    main_tp_consumer.cpp
)

set_target_properties(openaa_tire_pressure_consumer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TP_OUTPUT_DIRECTORY}"
    BUILD_RPATH "$ORIGIN/../../lib"
    INSTALL_RPATH "$ORIGIN/../../lib"
)

target_include_directories(openaa_tire_pressure_consumer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(openaa_tire_pressure_consumer
    PRIVATE
        ara_exec_lib
        ara_com_lib
        ara_log_lib
)

target_compile_definitions(openaa_tire_pressure_consumer PRIVATE
    OPEN_AA_TIRE_PRESSURE_SERVICE_MANIFEST="${TP_OUTPUT_DIRECTORY}/manifests/service_instance_manifest.json"
)

add_dependencies(openaa_tire_pressure_provider openaa_tire_pressure_manifest)
add_dependencies(openaa_tire_pressure_consumer openaa_tire_pressure_manifest)
