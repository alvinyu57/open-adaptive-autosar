# ---------------------------------------------------------------------------
# ara_exec_lib — static library consumed by Adaptive Applications
# Provides the ara::exec API surface and the in-process runtime stub.
# ---------------------------------------------------------------------------
add_library(ara_exec_lib SHARED
    em_runtime_state.cpp
    exec_error_domain.cpp
    execution_client.cpp
    function_group.cpp
    function_group_state.cpp
    state_client.cpp
)

target_include_directories(ara_exec_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include   # exposes include/ara/exec/*.h to consumers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}     # grants access to em_runtime_state.hpp
)

target_link_libraries(ara_exec_lib
    PUBLIC
        ara_core_lib
)

set_target_properties(ara_exec_lib PROPERTIES OUTPUT_NAME ara_exec)

target_compile_features(ara_exec_lib PUBLIC cxx_std_20)

# ---------------------------------------------------------------------------
# ara_exec — the Execution Manager process
# Links against ara_exec_lib and ara_log_lib; not consumed by anything else.
# ---------------------------------------------------------------------------
add_executable(ara_exec
    main.cpp
)

target_include_directories(ara_exec
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include   # access to include/ara/**/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}     # access to internal headers e.g. em_runtime_state.hpp
)

target_link_libraries(ara_exec
    PRIVATE
        ara_exec_lib
        ara_log_lib
)

target_compile_features(ara_exec PRIVATE cxx_std_20)