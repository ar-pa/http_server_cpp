cmake_minimum_required(VERSION 3.17)
project(httpserver)

set(CMAKE_CXX_FLAGS -O2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -pthread)

add_executable(httpserver main.cpp)