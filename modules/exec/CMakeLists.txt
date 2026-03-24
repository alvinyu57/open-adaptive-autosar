add_library(lib_openaa_exec
    src/execution_manager.cpp
)

set_target_properties(lib_openaa_exec PROPERTIES
    PREFIX ""
)

target_include_directories(lib_openaa_exec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lib_openaa_exec
    PUBLIC
        lib_openaa_core
)

target_compile_features(lib_openaa_exec PUBLIC cxx_std_20)

add_executable(openaa_exec
    src/main.cpp
)

target_link_libraries(openaa_exec
    PRIVATE
        lib_openaa_exec
)
