add_library(lib_openaa_core
    src/application.cpp
)

set_target_properties(lib_openaa_core PROPERTIES
    PREFIX ""
)

target_include_directories(lib_openaa_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(lib_openaa_core PUBLIC cxx_std_20)
