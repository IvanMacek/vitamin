# Vitamin [![Build Status][travis status]][travis]

Supposed to be a 3D FPS Game Engine but let's see if any of that becomes true.

## Windows dependencies

    choco install winlibs  
    choco install cmake  

## Build

    mkdir build  
    cd build  
    cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  
    make  
