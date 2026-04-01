add_executable(openaa_app_manifest_demo
    main.cpp
)

set_target_properties(openaa_app_manifest_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OPEN_AA_APP_OUTPUT_DIRECTORY}/02_app_manifest"
)

target_include_directories(openaa_app_manifest_demo
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(openaa_app_manifest_demo
    PRIVATE
        ara_exec_lib
)

target_compile_features(openaa_app_manifest_demo PRIVATE cxx_std_20)