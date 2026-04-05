cmake_minimum_required(VERSION 3.20)
project(open_adaptive_autosar VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH $ORIGIN/../lib)
set(CMAKE_INSTALL_RPATH $ORIGIN/../lib)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(OPEN_AA_BUILD_APPS "Build applications" OFF)
option(OPEN_AA_BUILD_TESTS "Build test targets" OFF)

find_package(Python3 REQUIRED)

add_custom_target(process_platform_manifests ALL
    COMMAND echo "Processing ARXML manifests..."
)

add_custom_command(
    TARGET  process_platform_manifests PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/verify_arxml.py
            ${CMAKE_SOURCE_DIR}/src/manifests/machine_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
    DEPENDS ${CMAKE_SOURCE_DIR}/src/manifests/machine_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
)

add_custom_command(
    TARGET  process_platform_manifests PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/arxml2json.py
            ${CMAKE_SOURCE_DIR}/src/manifests/machine_manifest.arxml
            ${CMAKE_BINARY_DIR}/manifests/machine_manifest.json
    DEPENDS ${CMAKE_SOURCE_DIR}/src/manifests/machine_manifest.arxml
)

add_custom_command(
    TARGET  process_platform_manifests PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/verify_arxml.py
            ${CMAKE_SOURCE_DIR}/src/manifests/execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
    DEPENDS ${CMAKE_SOURCE_DIR}/src/manifests/execution_manifest.arxml
            ${CMAKE_SOURCE_DIR}/src/manifests/schema/AUTOSAR_00054.xsd
)

add_custom_command(
    TARGET  process_platform_manifests PRE_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/manifests/arxml2json.py
            ${CMAKE_SOURCE_DIR}/src/manifests/execution_manifest.arxml
            ${CMAKE_BINARY_DIR}/manifests/execution_manifest.json
    DEPENDS ${CMAKE_SOURCE_DIR}/src/manifests/execution_manifest.arxml
)

add_subdirectory(src)

# [WORKAROUND] before manifest are correctly processed
##########################################################
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/manifests/machine_manifest.json
#      DESTINATION ${CMAKE_BINARY_DIR}/manifests/machine_manifest.json)

add_custom_command(
    TARGET  process_platform_manifests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/src/manifests/machine_manifest.json
            ${CMAKE_BINARY_DIR}/manifests/machine_manifest.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/manifests/machine_manifest.json
)

##########################################################

if(OPEN_AA_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(OPEN_AA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
