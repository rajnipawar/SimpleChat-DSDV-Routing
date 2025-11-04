#!/bin/bash

# Build script for SimpleChat PA3

echo "Building SimpleChat PA3..."

# Create build directory if it doesn't exist
mkdir -p build

# Change to build directory
cd build

# Run cmake
echo "Running CMake..."
cmake .. || { echo "CMake failed"; exit 1; }

# Build the project
echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || { echo "Build failed"; exit 1; }

echo ""
echo "Build successful!"
echo "Executable: build/SimpleChat_PA3"
echo ""
echo "To run a single node:"
echo "  ./build/SimpleChat_PA3 -p 9001"
echo ""
echo "Or use the launch scripts:"
echo "  ./scripts/launch_3_nodes.sh"
echo "  ./scripts/test_nat_traversal.sh"
