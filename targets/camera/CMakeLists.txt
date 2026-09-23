cmake_minimum_required(VERSION 3.3)
project(Camera)

set(CMAKE_CXX_STANDARD 20)

add_executable(Camera camera.cpp)
target_include_directories(Camera PRIVATE ${CMAKE_SOURCE_DIR}/libs)
