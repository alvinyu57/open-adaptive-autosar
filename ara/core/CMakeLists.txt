add_library(ara_core INTERFACE)

target_include_directories(ara_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ara_core INTERFACE cxx_std_20)
