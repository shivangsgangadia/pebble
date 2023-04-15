cmake_minimum_required(VERSION 2.8)

project(CommandEngine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -lm -lpigpio -lrt")

add_executable(
CommandEngine
main.cpp
ServoDriver.h
ServoDriver.cpp
gait.h
gait.cpp
)

target_link_libraries(CommandEngine PRIVATE pigpio)

