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

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/manifests
     DESTINATION ${OPEN_AA_APP_OUTPUT_DIRECTORY}/01_hello_world)
