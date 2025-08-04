# SocketCAN Library

Thư viện C++ đơn giản để giao tiếp với CAN bus trên Linux sử dụng SocketCAN.

## Tính năng

- **SocketCanIntf**: Interface chính để gửi/nhận CAN frames
- **EpollEventLoop**: Event loop hiệu suất cao dựa trên epoll
- **EpollEvent**: Event wrapper sử dụng eventfd
- Hỗ trợ non-blocking I/O
- Callback-based frame processing

## Cấu trúc project

```
socket_can/
├── include/socket_can/     # Header files
│   ├── socket_can.hpp
│   └── epoll_event_loop.hpp
├── src/                    # Implementation
│   ├── socket_can.cpp
│   └── epoll_event_loop.cpp
├── test/                   # Test files
│   ├── test_socket_can.cpp
│   ├── integration_test.cpp
│   └── CMakeLists.txt
└── CMakeLists.txt
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Chạy tests

```bash
# Chạy tất cả tests với script tự động
./run_can_tests.sh

# Hoặc chạy từng test riêng lẻ:

# Unit tests (không cần CAN interface)
./build/test/test_socket_can

# Integration test
./build/test/integration_test

# CAN monitor (đọc frames real-time)
./build/test/can_monitor vcan0

# CAN sender (gửi test frames)
./build/test/can_sender_test vcan0

# CAN reader (log chi tiết frames)
./build/test/can_reader_test vcan0

# Combined read/write test (tự test)
./build/test/can_read_write_test vcan0
```

## Sử dụng cơ bản

```cpp
#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"

// Tạo event loop
EpollEventLoop loop;

// Tạo CAN interface
SocketCanIntf can_socket;

// Callback xử lý frame nhận được
auto frame_handler = [](const can_frame& frame) {
    std::cout << "Received CAN ID: 0x" << std::hex << frame.can_id << std::endl;
};

// Khởi tạo với interface "can0"
if (can_socket.init("can0", &loop, frame_handler)) {
    // Gửi frame
    can_frame frame = {};
    frame.can_id = 0x123;
    frame.can_dlc = 8;
    // ... set data ...
    
    can_socket.send_can_frame(frame);
    
    // Chạy event loop
    loop.run_until_empty();
    
    // Dọn dẹp
    can_socket.deinit();
}
```

## Thiết lập CAN interface (Virtual)

Để test, bạn có thể tạo virtual CAN interface:

```bash
# Load module
sudo modprobe vcan

# Tạo virtual interface
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Test với cansend/canreceive
cansend vcan0 123#DEADBEEF
candump vcan0
```

## Test Programs

### 1. Unit Tests (`test_socket_can`)
- Tests cơ bản cho tất cả components
- Không cần CAN interface thật
- Kiểm tra API và error handling

### 2. Integration Test (`integration_test`) 
- Test performance của event loop
- Test với CAN interface (nếu có)
- Benchmark tốc độ xử lý events

### 3. CAN Monitor (`can_monitor`)
- Monitor real-time CAN frames
- Hiển thị timestamp và frame details
- Chỉ đọc, không gửi

### 4. CAN Sender (`can_sender_test`)
- Gửi các loại CAN frames khác nhau
- Demo standard/extended/RTR frames
- Test sequence và random data

### 5. CAN Reader (`can_reader_test`)
- Log chi tiết từng CAN frame
- Phân tích HEX, DEC, ASCII
- Hỗ trợ Ctrl+C graceful shutdown

### 6. Combined Read/Write (`can_read_write_test`)
- **Đọc và ghi trong cùng 1 process**
- Tự động gửi test frames
- Monitor frames đã gửi và nhận
- Self-testing với statistics

## API Reference

### SocketCanIntf

- `bool init(interface, event_loop, frame_processor)` - Khởi tạo CAN socket
- `void deinit()` - Dọn dẹp resources
- `bool send_can_frame(const can_frame&)` - Gửi CAN frame
- `bool read_nonblocking()` - Đọc frame (internal use)

### EpollEventLoop

- `bool register_event(evt_id, fd, events, callback)` - Đăng ký event
- `bool deregister_event(evt_id)` - Hủy đăng ký event
- `bool run_until_empty()` - Chạy event loop

### EpollEvent

- `bool init(event_loop, callback)` - Khởi tạo event
- `void deinit()` - Dọn dẹp event
- `bool set()` - Trigger event

## Requirements

- Linux với SocketCAN support
- CMake 3.10+
- C++17 compiler
- Kernel headers (linux/can.h)

## Notes

- Thư viện yêu cầu CAN interface thật hoặc virtual để hoạt động
- Tests có thể fail nếu không có CAN interface, điều này là bình thường
- Sử dụng non-blocking sockets để tránh deadlock