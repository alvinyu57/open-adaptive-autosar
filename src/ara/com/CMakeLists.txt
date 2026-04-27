add_library(ara_com_lib SHARED
    com_error_domain.cpp
    com_runtime_state.cpp
    runtime.cpp
    shared_memory_ipc.cpp
    someip_binding_runtime.cpp
)

target_include_directories(ara_com_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ara_com_lib
    PUBLIC
        ara_core_lib
    PRIVATE
        vsomeip3
)

set_target_properties(ara_com_lib PROPERTIES OUTPUT_NAME ara_com)