cmake_minimum_required(VERSION 3.25)

find_package(Geode REQUIRED)
project(GDFrameWindowCounter VERSION 1.0.0 LANGUAGES CXX)

# 修改：將綁定的 Mod ID 同步更新為 nangd
add_geode_mod(
    nangd.frame_window_counter
    SOURCES 
        src/main.cpp
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
