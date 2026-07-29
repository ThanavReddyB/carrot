cmake_minimum_required(VERSION 3.20)

project(Carrot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(carrot
    src/main.cpp
    src/init.cpp
)

target_include_directories(carrot PRIVATE include)