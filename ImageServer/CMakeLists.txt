cmake_minimum_required(VERSION 2.8)

project(ImageServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -pthread -lm -lwiringPi -lwiringPiDev")



add_executable(
ImageServer
main.cpp
)

#find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
#target_link_libraries(ImageServer ${WIRINGPI_LIBRARIES})
