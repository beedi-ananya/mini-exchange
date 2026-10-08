cmake_minimum_required(VERSION 3.14)
project(mini_exchange CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_executable(engine src/main.cpp)
