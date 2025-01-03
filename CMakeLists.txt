cmake_minimum_required(VERSION 3.16)

project(mhw-font)

set(CMAKE_CXX_STANDARD 23)
set(MHW_FONT_UTIL_SOURCE src/main.cpp)

add_executable(mhw-font-util ${MHW_FONT_UTIL_SOURCE})
target_include_directories(mhw-font-util PRIVATE include)
