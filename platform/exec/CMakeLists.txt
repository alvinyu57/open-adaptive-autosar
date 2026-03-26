add_library(openaa_platform_exec
    src/execution_manager.cpp
)

set_target_properties(openaa_platform_exec PROPERTIES
    PREFIX ""
)

target_include_directories(openaa_platform_exec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(openaa_platform_exec
    PUBLIC
        ara_exec
        openaa_platform_core
)

target_compile_features(openaa_platform_exec PUBLIC cxx_std_20)

add_executable(openaa_exec
    src/main.cpp
)

target_link_libraries(openaa_exec
    PRIVATE
        openaa_platform_exec
)
