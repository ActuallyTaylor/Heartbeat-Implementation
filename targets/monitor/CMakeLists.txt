cmake_minimum_required(VERSION 3.3)
project(Monitor)

set(CMAKE_CXX_STANDARD 20)

add_executable(Monitor monitor.cpp)
target_include_directories(Monitor PRIVATE ${CMAKE_SOURCE_DIR}/libs)
