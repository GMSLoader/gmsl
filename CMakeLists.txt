cmake_minimum_required(VERSION 3.8)

project("gmsl")

set(OutDir "${CMAKE_SOURCE_DIR}/out")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory("gmsl-loader")
add_subdirectory("gmsl-patcher")
add_subdirectory("gmsl-interop")