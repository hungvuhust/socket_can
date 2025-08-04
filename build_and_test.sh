#!/bin/bash

# Script build và test cho SocketCAN library

set -e  # Exit on error

echo "=== Building SocketCAN Library ==="

# Tạo build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure với CMake
echo "Configuring with CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "=== Running Tests ==="

# Chạy unit tests
echo "Running unit tests..."
if [ -f "test/test_socket_can" ]; then
    ./test/test_socket_can
else
    echo "Error: test_socket_can not found"
    exit 1
fi

echo ""

# Chạy integration tests
echo "Running integration tests..."
if [ -f "test/integration_test" ]; then
    ./test/integration_test
else
    echo "Error: integration_test not found"
    exit 1
fi

# Chạy CAN reader test
echo "Running CAN reader test..."
if [ -f "test/can_reader_test" ]; then
    ./test/can_reader_test
else
    echo "Error: can_reader_test not found"
    exit 1
fi

# Chạy CAN sender test
echo "Running CAN sender test..."
if [ -f "test/can_sender_test" ]; then
    ./test/can_sender_test
else
    echo "Error: can_sender_test not found"
    exit 1
fi



echo ""
echo "=== Build and Test Completed Successfully! ==="

# Hiện thị thông tin về CAN interfaces available
echo ""
echo "=== Available Network Interfaces ==="
ip link show | grep -E "(can|vcan)" || echo "No CAN interfaces found"

echo ""
echo "To create a virtual CAN interface for testing:"
echo "  sudo modprobe vcan"
echo "  sudo ip link add dev vcan0 type vcan"
echo "  sudo ip link set up vcan0"
echo ""
echo "Then test with:"
echo "  cansend vcan0 123#DEADBEEF"
echo "  candump vcan0"