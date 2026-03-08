#!/bin/bash

echo "Building MMO Server..."

mkdir -p build
cd build

echo "Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "CMake failed!"
    exit 1
fi

echo "Building..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
echo "Binary: build/bin/mmo_server"

cd ..
