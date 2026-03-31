add_library(ara_core
    abort.cpp
    core_error_domain.cpp
    initialization.cpp
)

target_include_directories(ara_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(ara_core PROPERTIES
    PREFIX ""
)

target_link_libraries(ara_core
    PUBLIC
        ara_log
)

target_compile_features(ara_core PUBLIC cxx_std_20)
