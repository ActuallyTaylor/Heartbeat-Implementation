cmake_minimum_required(VERSION 4.3)
project(Heartbeat_Implementation)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(targets/bus)
add_subdirectory(targets/camera)
add_subdirectory(targets/monitor)
