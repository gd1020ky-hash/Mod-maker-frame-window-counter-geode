cmake_minimum_required(VERSION 3.25)

# 1. Enforce that the platform environment must be Android
if(NOT ANDROID)
    message(FATAL_ERROR "❌ This Frame Window Counter mod is configured for Android ONLY. Aborting build.")
endif()

find_package(Geode REQUIRED)
project(GDFrameWindowCounter VERSION 1.0.0 LANGUAGES CXX)

add_geode_mod(
    yourname.frame_window_counter
    SOURCES 
        src/main.cpp
)

# 2. Package sound mechanics explicitly for Android binary bundles
target_resources(yourname.frame_window_counter
    RESOURCES
        resources/frame_perfect.mp3
        resources/tight_window.mp3
        resources/normal_click.mp3
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
