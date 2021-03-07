# Vitamin [![Build Status](https://travis-ci.com/IvanMacek/vitamin.svg?branch=main)](https://travis-ci.com/IvanMacek/vitamin)

Supposed to be a 3D FPS Game Engine but let's see if any of that really happens.

## Tool dependencies

Required:

    choco install winlibs
    choco install cmake
    choco install conan 

Optional:

    choco install vulkan-sdk
    choco install renderdoc


## Build

Build is assumed to be executed from the _build_ directory:

    cd build

JFrog Conan is used for package managment. You can install all the packages by running:

    conan install .. --build=missing

Generate cmake build project:

    cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

Run the build:
   
    make
