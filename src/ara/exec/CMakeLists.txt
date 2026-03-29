add_library(ara_exec
    src/application_manifest_loader.cpp
    src/execution_manager.cpp
    src/manifest_path.cpp
)

target_include_directories(ara_exec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(ara_exec
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ara_exec
    PUBLIC
        ara_core
)

set_target_properties(ara_exec PROPERTIES
    PREFIX ""
)

target_compile_features(ara_exec PUBLIC cxx_std_20)

add_executable(ara_exec_demo
    src/main.cpp
)

target_link_libraries(ara_exec_demo
    PRIVATE
        ara_exec
)

configure_file(
    manifests/diagnostics.json
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/diagnostics.json
    COPYONLY
)
