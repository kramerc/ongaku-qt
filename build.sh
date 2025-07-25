#!/bin/bash

# Build script for Ongaku

echo "Building Ongaku..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run './build/Ongaku' to start the application"
else
    echo "Build failed!"
    exit 1
fi
