cmake_minimum_required(VERSION 3.25)

# 移除原本會導致 GitHub 報錯的硬性限制，改由 Geode 核心套件接管
find_package(Geode REQUIRED)
project(GDFrameWindowCounter VERSION 1.0.0 LANGUAGES CXX)

add_geode_mod(
    yourname.frame_window_counter
    SOURCES 
        src/main.cpp
)

# 明確將鈴聲素材包綁定到編譯輸出中
target_resources(yourname.frame_window_counter
    RESOURCES
        resources/nan_bell.mp3
)

# 沿用你要求的 C++23 最高標準規格
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
