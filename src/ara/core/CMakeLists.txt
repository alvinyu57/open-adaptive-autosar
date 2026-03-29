add_library(ara_core
    src/application.cpp
)

target_include_directories(ara_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(ara_core PROPERTIES
    PREFIX ""
)

target_compile_features(ara_core PUBLIC cxx_std_20)
