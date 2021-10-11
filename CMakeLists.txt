cmake_minimum_required(VERSION 3.16)
project(server C)

set(CMAKE_C_STANDARD 11)
FIND_PACKAGE(Threads)
include_directories(.)

add_executable(server
        server.c IO.c IO.h server.h core.c core.h command.c command.h)
TARGET_LINK_LIBRARIES(server ${CMAKE_THREAD_LIBS_INIT})
add_definitions(-w)