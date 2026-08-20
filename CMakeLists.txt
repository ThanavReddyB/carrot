cmake_minimum_required(VERSION 3.20)

project(Carrot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(carrot
    src/main.cpp
    src/repository.cpp
    src/hash.cpp
)

target_include_directories(carrot PRIVATE include)