cmake_minimum_required(VERSION 3.3)
project(Bus)

set(CMAKE_CXX_STANDARD 20)

add_executable(Bus bus.cpp)
target_include_directories(Bus PRIVATE ${CMAKE_SOURCE_DIR}/libs)
