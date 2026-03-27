add_library(lib_openaa_example_hello_world
    src/hello_world_app.cpp
)

set_target_properties(lib_openaa_example_hello_world PROPERTIES
    PREFIX ""
)

target_include_directories(lib_openaa_example_hello_world
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lib_openaa_example_hello_world
    PUBLIC
        ara_core
        openaa_platform_core
)

target_compile_features(lib_openaa_example_hello_world PUBLIC cxx_std_20)

add_executable(openaa_example_hello_world_runner
    main.cpp
)

target_link_libraries(openaa_example_hello_world_runner
    PRIVATE
        openaa_platform_exec
        lib_openaa_example_hello_world
)
