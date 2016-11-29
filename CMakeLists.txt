cmake_minimum_required(VERSION 2.8)

project(VideoStreaming-SD)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_COMPILER clang++)


set(SOURCE_FILES main.cpp)
add_executable(ecosystem ${SOURCE_FILES})


find_package(Ice 3.6.0 REQUIRED Ice IceStorm)