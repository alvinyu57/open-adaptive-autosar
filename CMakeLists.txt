cmake_minimum_required(VERSION 3.20)
project(open_adaptive_autosar VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH $ORIGIN/../lib)
set(CMAKE_INSTALL_RPATH $ORIGIN/../lib)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(OPEN_AA_BUILD_APPS "Build applications" OFF)
option(OPEN_AA_BUILD_TESTS "Build test targets" OFF)

add_subdirectory(src)

# Copy manifests to build directory for EM to load
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/manifests/ 
     DESTINATION ${CMAKE_BINARY_DIR}/manifests)

if(OPEN_AA_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(OPEN_AA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
