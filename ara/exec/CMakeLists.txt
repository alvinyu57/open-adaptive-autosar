add_library(ara_exec INTERFACE)

target_include_directories(ara_exec
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ara_exec
    INTERFACE
        ara_core
)

target_compile_features(ara_exec INTERFACE cxx_std_20)
