file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create libara_exec as a library target
add_library(ara_exec_lib
    ${SRC_FILES}
)

target_include_directories(ara_exec_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ara_exec_lib
    PUBLIC
        ara_core_lib
)

set_target_properties(ara_exec_lib PROPERTIES OUTPUT_NAME ara_exec)

target_compile_features(ara_exec_lib PUBLIC cxx_std_20)

# # Create ara_exec as an executable target
# add_executable(ara_exec
#     ${SRC_FILES}
# )

# target_include_directories(ara_exec
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# target_link_libraries(ara_exec
#     PRIVATE
#         ara_exec_lib
# )

# target_compile_features(ara_exec PUBLIC cxx_std_20)