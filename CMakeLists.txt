cmake_minimum_required(VERSION 3.20)
project(open_adaptive_autosar VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(OPEN_AA_BUILD_APPS "Build applications" ON)
option(OPEN_AA_BUILD_TESTS "Build test targets" OFF)

add_subdirectory(ara)
add_subdirectory(platform)

if(OPEN_AA_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(OPEN_AA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
