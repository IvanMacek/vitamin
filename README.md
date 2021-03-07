# Windows dependencies

choco install winlibs
choco install cmake


# Build

mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make