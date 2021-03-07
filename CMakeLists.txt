cmake_minimum_required(VERSION 3.19)

# C++

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -Werror)

# Project

project(Vitamin)

file(GLOB SOURCES "./src/*.cpp" "./src/**/*.cpp")

add_executable(main ${SOURCES})
