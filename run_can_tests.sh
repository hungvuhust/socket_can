#!/bin/bash

# Script để chạy các test CAN khác nhau

set -e

echo "=== CAN Test Programs ==="

# Build trước
echo "Building all tests..."
if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ..
make -j$(nproc)

echo ""
echo "✅ Build completed successfully!"
echo ""

# Kiểm tra interface vcan0
echo "=== Checking CAN Interface ==="
if ip link show vcan0 &> /dev/null; then
    echo "✅ vcan0 interface exists"
else
    echo "❌ vcan0 interface not found"
    echo "Setting up virtual CAN interface..."
    echo "Run these commands as root:"
    echo "  sudo modprobe vcan"
    echo "  sudo ip link add dev vcan0 type vcan"
    echo "  sudo ip link set up vcan0"
    echo ""
    echo "Then run this script again."
    exit 1
fi

echo ""
echo "=== Available Test Programs ==="
echo ""

echo "1. 🧪 Unit Tests (test_socket_can)"
echo "   - Basic functionality tests"
echo "   - No CAN interface required"
echo "   Command: ./test/test_socket_can"
echo ""

echo "2. 🔧 Integration Test (integration_test)"
echo "   - Advanced integration testing"
echo "   - Tests event loop performance"
echo "   Command: ./test/integration_test"
echo ""

echo "3. 👁️  CAN Frame Monitor (can_monitor)"
echo "   - Real-time monitoring of CAN frames"
echo "   - Read-only, displays all incoming frames"
echo "   Command: ./test/can_monitor [interface]"
echo "   Example: ./test/can_monitor vcan0"
echo ""

echo "4. 📤 CAN Frame Sender (can_sender_test)"
echo "   - Sends various test CAN frames"
echo "   - Demonstrates different frame types"
echo "   Command: ./test/can_sender_test [interface]"
echo "   Example: ./test/can_sender_test vcan0"
echo ""

echo "5. 📖 CAN Frame Reader (can_reader_test)"
echo "   - Detailed frame logging with Ctrl+C support"
echo "   - Shows frame analysis and ASCII data"
echo "   Command: ./test/can_reader_test [interface]"
echo "   Example: ./test/can_reader_test vcan0"
echo ""

echo "6. 🔄 CAN Read/Write Test (can_read_write_test)"
echo "   - Combined reading AND writing in one process"
echo "   - Self-testing with automatic frame generation"
echo "   - Real-time monitoring of sent/received frames"
echo "   Command: ./test/can_read_write_test [interface]"
echo "   Example: ./test/can_read_write_test vcan0"
echo ""

echo "=== Quick Test Scenarios ==="
echo ""

# Function để chạy test với confirmation
run_test() {
    local test_name="$1"
    local command="$2"
    local description="$3"
    
    echo "📋 Test: $test_name"
    echo "📄 Description: $description"
    echo "🚀 Command: $command"
    echo ""
    read -p "Run this test? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Running: $command"
        echo "Press Ctrl+C to stop when ready..."
        echo ""
        eval $command
        echo ""
        echo "✅ Test completed: $test_name"
        echo ""
    else
        echo "⏭️  Skipped: $test_name"
        echo ""
    fi
}

# Scenario 1: Basic unit tests
run_test "Unit Tests" \
         "./test/test_socket_can" \
         "Run basic unit tests (no CAN interface needed)"

# Scenario 2: Integration test
run_test "Integration Test" \
         "./test/integration_test" \
         "Run integration tests with event loop performance testing"

# Scenario 3: Self-testing read/write
run_test "Self-Testing Read/Write" \
         "./test/can_read_write_test vcan0" \
         "Run combined read/write test that generates and monitors its own traffic"

# Scenario 4: Manual monitoring (nếu user muốn test với external tools)
echo "📋 Manual Testing Options:"
echo ""
echo "Option A: Monitor frames from external source"
echo "  Terminal 1: ./test/can_monitor vcan0"
echo "  Terminal 2: cansend vcan0 123#DEADBEEF"
echo ""
echo "Option B: Send test frames and monitor externally"
echo "  Terminal 1: candump vcan0"
echo "  Terminal 2: ./test/can_sender_test vcan0"
echo ""
echo "Option C: Detailed frame analysis"
echo "  Terminal 1: ./test/can_reader_test vcan0"
echo "  Terminal 2: cansend vcan0 456#48656C6C6F  # 'Hello'"
echo ""

echo "=== All Tests Available ==="
echo "All test programs are built and ready in ./test/ directory"
echo "Use './test/program_name --help' for more options"
echo ""
echo "🎉 Happy testing!"