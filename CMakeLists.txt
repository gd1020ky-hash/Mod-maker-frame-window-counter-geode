cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(OPFrameWindowCounter VERSION 1.0.0 LANGUAGES CXX)

find_package(Geode REQUIRED)

add_geode_mod(
    fwc_nandl_fb
    "1.0.0"
    src/main.cpp
)

target_include_directories(fwc_nandl_fb PRIVATE src)
