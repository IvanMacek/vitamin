# Vitamin [![Build Status](https://travis-ci.com/IvanMacek/vitamin.svg?branch=main)](https://travis-ci.com/IvanMacek/vitamin)

Supposed to be a 3D FPS Game Engine but let's see if any of that really happens.

## Tool dependencies

    choco install winlibs
    choco install cmake
    choco install conan 

## Build

All of the build commands are 

JFrog Conan is used for package managment. You can install all the packages by running:

    cd build
    conan install .. --build=missing

## Build

    cd build
    cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    make
