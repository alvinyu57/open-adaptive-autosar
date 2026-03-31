add_library(ara_core_lib
    abort.cpp
    core_error_domain.cpp
    initialization.cpp
)

target_include_directories(ara_core_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(ara_core_lib
    PUBLIC
        ara_log_lib
)

set_target_properties(ara_core_lib PROPERTIES OUTPUT_NAME ara_core)

target_compile_features(ara_core_lib PUBLIC cxx_std_20)
