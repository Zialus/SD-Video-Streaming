cmake_minimum_required(VERSION 2.8)

project(VideoStreaming-SD)

set(CMAKE_VERBOSE_MAKEFILE true)

message("COMPILER defined as:" ENV{COMPILER})
if(NOT DEFINED ENV{COMPILER})
    set(ENV{COMPILER} clang++)
    message(STATUS "COMPILER environment variable defined as clang++")
endif()


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER $ENV{COMPILER})

find_package(Ice REQUIRED Ice IceUtil IceStorm)

file(GLOB server_HDR "${PROJECT_SOURCE_DIR}/server_src/*.h")
file(GLOB server_SRC "${PROJECT_SOURCE_DIR}/server_src/*.cpp")

file(GLOB client_HDR "${PROJECT_SOURCE_DIR}/client_src/*.h")
file(GLOB client_SRC "${PROJECT_SOURCE_DIR}/client_src/*.cpp")

file(GLOB portal_HDR "${PROJECT_SOURCE_DIR}/portal_src/*.h")
file(GLOB portal_SRC "${PROJECT_SOURCE_DIR}/portal_src/*.cpp")

file(GLOB ice_SLICE "${PROJECT_SOURCE_DIR}/ice/*.ice")

string(REPLACE ".ice" ".cpp" ice_SRC ${ice_SLICE})
string(REPLACE ".ice" ".h" ice_HDR ${ice_SLICE})

add_custom_command(
        OUTPUT ${ice_HDR} ${ice_SRC}
        COMMAND ${Ice_SLICE2CPP_EXECUTABLE} ${ice_SLICE}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ice
)

include_directories(server ${PROJECT_SOURCE_DIR}/ice ${PROJECT_SOURCE_DIR}/server ${Ice_INCLUDE_DIRS})
include_directories(client ${PROJECT_SOURCE_DIR}/ice ${PROJECT_SOURCE_DIR}/client ${Ice_INCLUDE_DIRS})
include_directories(portal ${PROJECT_SOURCE_DIR}/ice ${PROJECT_SOURCE_DIR}/client ${Ice_INCLUDE_DIRS})

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/executables)
add_executable(server ${server_HDR} ${server_SRC} ${ice_HDR} ${ice_SRC})
add_executable(client ${client_HDR} ${client_SRC} ${ice_HDR} ${ice_SRC})
add_executable(portal ${portal_HDR} ${portal_SRC} ${ice_HDR} ${ice_SRC})

LINK_DIRECTORIES(/usr/lib/x86_64-linux-gnu/c++11/)

target_link_libraries(server ${Ice_LIBRARIES} pthread)
target_link_libraries(client ${Ice_LIBRARIES} pthread)
target_link_libraries(portal ${Ice_LIBRARIES} pthread)