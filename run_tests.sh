#!/bin/bash

echo "Building and running R-Type tests..."

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target rtype_tests -j$(nproc)

echo ""
echo "Running tests..."
./tests/rtype_tests --gtest_color=yes

echo ""
echo "Test execution completed !"