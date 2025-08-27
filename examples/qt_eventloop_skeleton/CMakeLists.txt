cmake_minimum_required(VERSION 3.12)
project(qt_eventloop_skeleton CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC/Clang/GCC 常用警告开关
if(MSVC)
	add_compile_options(/W4 /permissive-)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()


add_executable(qt_eventloop_skeleton
main.cpp
)