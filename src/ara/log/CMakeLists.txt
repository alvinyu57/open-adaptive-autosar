add_library(ara_log
    logger.cpp
)

target_include_directories(ara_log
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(ara_log PROPERTIES
    PREFIX ""
)

target_compile_features(ara_log PUBLIC cxx_std_20)
