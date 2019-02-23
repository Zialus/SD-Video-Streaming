cmake_minimum_required(VERSION 3.0)

project(VideoStreaming-SD)

set(CMAKE_CXX_STANDARD 14)

find_package(Ice REQUIRED Ice++11 IceStorm++11)

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
        DEPENDS ${ice_SLICE}
        COMMENT "Running SLICE2CPP..."
)

add_custom_target(
        IceGenerated
        DEPENDS ${ice_HDR} ${ice_SRC}
)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/executables)

add_compile_options("-DICE_CPP11_MAPPING")

add_executable(server ${server_HDR} ${server_SRC} ${ice_HDR} ${ice_SRC} auxiliary/Auxiliary.h)
add_executable(client ${client_HDR} ${client_SRC} ${ice_HDR} ${ice_SRC} auxiliary/Auxiliary.h)
add_executable(portal ${portal_HDR} ${portal_SRC} ${ice_HDR} ${ice_SRC} auxiliary/Auxiliary.h)

include_directories(server ${PROJECT_SOURCE_DIR}/ice)
include_directories(client ${PROJECT_SOURCE_DIR}/ice)
include_directories(portal ${PROJECT_SOURCE_DIR}/ice)

if (UNIX)
    set(MAYBE_PTHREADS "pthread")
endif (UNIX)

target_link_libraries(server Ice::Ice++11 ${MAYBE_PTHREADS})
target_link_libraries(client Ice::Ice++11 ${MAYBE_PTHREADS} Ice::IceStorm++11 readline)
target_link_libraries(portal Ice::Ice++11 ${MAYBE_PTHREADS} Ice::IceStorm++11)

add_dependencies(server IceGenerated)
add_dependencies(client IceGenerated)
add_dependencies(portal IceGenerated)
