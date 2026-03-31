add_library(ara_log_lib
    logger.cpp
)

target_include_directories(ara_log_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(ara_log_lib PROPERTIES OUTPUT_NAME ara_log)

target_compile_features(ara_log_lib PUBLIC cxx_std_20)
