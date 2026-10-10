cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(OPFrameWindowCounter VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

# Automatically fetch Geode SDK CMake macros if not present
if(NOT DEFINED GEODE_SDK_PATH)
    if(DEFINED ENV{GEODE_SDK})
        set(GEODE_SDK_PATH $ENV{GEODE_SDK})
    else()
        message(FATAL_ERROR "Geode SDK path not found. Build Geode Mod Action must run first.")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${GEODE_SDK_PATH}/cmake")
include(Geode)

add_geode_mod(
    fwc_nandl_fb
    "1.0.0"
    src/main.cpp
)

target_include_directories(fwc_nandl_fb PRIVATE src)
