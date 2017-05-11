cmake_minimum_required(VERSION 3.7)
project(http)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

set(SOURCE_FILES
        http_parser.c
        http_parser.h
        LICENSE
        main.cpp
        README.md
        RequestParser.h
        tool.h)

add_executable(http ${SOURCE_FILES})