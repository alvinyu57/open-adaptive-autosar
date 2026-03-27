add_library(openaa_platform_core
    src/application.cpp
)

set_target_properties(openaa_platform_core PROPERTIES
    PREFIX ""
)

target_include_directories(openaa_platform_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(openaa_platform_core
    PUBLIC
        ara_core
)

target_compile_features(openaa_platform_core PUBLIC cxx_std_20)
