add_executable(openaa_example_hello_world_runner
    main.cpp
)

target_link_libraries(openaa_example_hello_world_runner
    PRIVATE
        libara_exec
)

configure_file(
    manifests/hello_world.json
    ${CMAKE_CURRENT_BINARY_DIR}/manifests/hello_world.json
    COPYONLY
)
