# Vitamin [![Build Status](https://travis-ci.com/IvanMacek/vitamin.svg?branch=main)](https://travis-ci.com/IvanMacek/vitamin)

Supposed to be a 3D FPS Game Engine but let's see if any of that really happens.

## Manual dependencies

    choco install winlibs
    choco install vulkan-sdk
    choco install cmake
    choco install conan 

## Build

Build is assumed to be executed from the _build_ directory:

    mkdir build
    cd build

JFrog Conan is used for package managment. You can install all the packages by running:

    conan install .. --build=missing

Generate cmake build project:

    cmake .. -G "Unix Makefiles" 

Run the build:
   
    make
