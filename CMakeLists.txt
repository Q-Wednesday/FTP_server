cmake_minimum_required(VERSION 3.16)
project(FTP_server C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(FTP_server
        server.c IO.c IO.h server.h core.c core.h command.c command.h)
