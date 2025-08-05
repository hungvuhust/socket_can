# Hướng dẫn sử dụng thư viện SocketCAN

## Tổng quan

Thư viện SocketCAN là một thư viện C++ đơn giản và hiệu quả để giao tiếp với CAN bus trên Linux. Thư viện cung cấp interface dễ sử dụng với event-driven architecture dựa trên epoll, cho phép xử lý CAN frames một cách bất đồng bộ và hiệu suất cao.

## Tính năng chính

- ✅ **SocketCanIntf**: Interface chính để gửi và nhận CAN frames
- ✅ **EpollEventLoop**: Event loop hiệu suất cao dựa trên epoll
- ✅ **EpollEvent**: Event wrapper sử dụng eventfd
- ✅ Hỗ trợ non-blocking I/O
- ✅ Callback-based frame processing
- ✅ Hỗ trợ cả Standard và Extended CAN frames
- ✅ Hỗ trợ RTR (Remote Transmission Request) frames
- ✅ Thread-safe và scalable

## Cài đặt và Build

### Yêu cầu hệ thống

- Linux kernel với SocketCAN support
- CMake 3.10 trở lên
- C++17 compiler (GCC 7+ hoặc Clang 5+)
- Kernel headers (`linux/can.h`)

### Build thư viện

```bash
# Clone repository (nếu chưa có)
git clone <repository-url>
cd socket_can

# Tạo thư mục build
mkdir build && cd build

# Configure với CMake
cmake ..

# Build
make -j$(nproc)

# Chạy tests (tùy chọn)
make test
```

## Sử dụng cơ bản

### 1. Khởi tạo CAN Interface

```cpp
#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>

int main() {
    // Tạo event loop
    EpollEventLoop event_loop;
    
    // Tạo CAN interface
    SocketCanIntf can_socket;
    
    // Định nghĩa callback xử lý frame nhận được
    auto frame_processor = [](const can_frame& frame) {
        std::cout << "Nhận được CAN frame ID: 0x" 
                  << std::hex << (frame.can_id & CAN_EFF_MASK) 
                  << std::endl;
    };
    
    // Khởi tạo với interface "can0"
    if (!can_socket.init("can0", &event_loop, frame_processor)) {
        std::cerr << "Không thể khởi tạo CAN interface" << std::endl;
        return -1;
    }
    
    std::cout << "CAN interface đã được khởi tạo thành công" << std::endl;
    
    // Chạy event loop
    event_loop.run_until_empty();
    
    // Dọn dẹp
    can_socket.deinit();
    
    return 0;
}
```

### 2. Gửi CAN Frames

```cpp
// Gửi Standard CAN frame
can_frame frame = {};
frame.can_id = 0x123;           // CAN ID
frame.can_dlc = 8;              // Data length
frame.data[0] = 0x01;           // Data bytes
frame.data[1] = 0x02;
frame.data[2] = 0x03;
frame.data[3] = 0x04;
frame.data[4] = 0x05;
frame.data[5] = 0x06;
frame.data[6] = 0x07;
frame.data[7] = 0x08;

if (can_socket.send_can_frame(frame)) {
    std::cout << "Đã gửi CAN frame thành công" << std::endl;
} else {
    std::cerr << "Lỗi khi gửi CAN frame" << std::endl;
}

// Gửi Extended CAN frame
can_frame ext_frame = {};
ext_frame.can_id = 0x18FF1234 | CAN_EFF_FLAG;  // Extended ID
ext_frame.can_dlc = 4;
ext_frame.data[0] = 0xAA;
ext_frame.data[1] = 0xBB;
ext_frame.data[2] = 0xCC;
ext_frame.data[3] = 0xDD;

can_socket.send_can_frame(ext_frame);

// Gửi RTR (Remote Transmission Request) frame
can_frame rtr_frame = {};
rtr_frame.can_id = 0x456 | CAN_RTR_FLAG;  // RTR flag
rtr_frame.can_dlc = 8;  // DLC cho RTR frame

can_socket.send_can_frame(rtr_frame);
```

### 3. Xử lý CAN Frames nâng cao

```cpp
auto advanced_frame_processor = [](const can_frame& frame) {
    // Phân tích CAN ID
    bool is_extended = frame.can_id & CAN_EFF_FLAG;
    bool is_rtr = frame.can_id & CAN_RTR_FLAG;
    
    uint32_t can_id;
    if (is_extended) {
        can_id = frame.can_id & CAN_EFF_MASK;
        std::cout << "Extended CAN ID: 0x" << std::hex << can_id << std::endl;
    } else {
        can_id = frame.can_id & CAN_SFF_MASK;
        std::cout << "Standard CAN ID: 0x" << std::hex << can_id << std::endl;
    }
    
    if (is_rtr) {
        std::cout << "RTR frame với DLC: " << (int)frame.can_dlc << std::endl;
    } else {
        std::cout << "Data frame với " << (int)frame.can_dlc << " bytes:" << std::endl;
        
        // In data bytes
        for (int i = 0; i < frame.can_dlc; i++) {
            std::cout << "0x" << std::hex << (int)frame.data[i] << " ";
        }
        std::cout << std::endl;
        
        // In ASCII representation
        std::cout << "ASCII: ";
        for (int i = 0; i < frame.can_dlc; i++) {
            char c = frame.data[i];
            if (c >= 32 && c <= 126) {
                std::cout << c;
            } else {
                std::cout << ".";
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "---" << std::endl;
};
```

## Thiết lập CAN Interface

### Virtual CAN Interface (cho testing)

```bash
# Load vcan module
sudo modprobe vcan

# Tạo virtual CAN interface
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Kiểm tra interface
ip link show vcan0

# Test với cansend/candump
cansend vcan0 123#DEADBEEF
candump vcan0
```

### Real CAN Interface

```bash
# Load CAN module (nếu cần)
sudo modprobe can
sudo modprobe can_raw

# Thiết lập CAN interface (ví dụ với can0)
sudo ip link set can0 type can bitrate 500000
sudo ip link set up can0

# Kiểm tra trạng thái
ip -details link show can0
```

## Các chương trình test

### 1. CAN Monitor - Giám sát real-time

```bash
# Monitor tất cả frames trên interface
./build/test/can_monitor vcan0

# Output mẫu:
# [14:30:25.123] Frame #     1 - ID: 0x123 (STD) DLC: 8 Data: [0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08] ASCII: "........"
```

### 2. CAN Sender - Gửi test frames

```bash
# Gửi các loại frames khác nhau
./build/test/can_sender_test vcan0

# Gửi frame cụ thể
cansend vcan0 123#DEADBEEF
cansend vcan0 456#R
```

### 3. CAN Reader - Đọc và log chi tiết

```bash
# Đọc frames với log chi tiết
./build/test/can_reader_test vcan0
```

### 4. Combined Read/Write - Tự test

```bash
# Chương trình tự test đọc/ghi
./build/test/can_read_write_test vcan0
```

## API Reference

### SocketCanIntf Class

#### Methods

- **`bool init(const std::string& interface, EpollEventLoop* event_loop, FrameProcessor frame_processor)`**
  - Khởi tạo CAN socket interface
  - `interface`: Tên CAN interface (ví dụ: "can0", "vcan0")
  - `event_loop`: Pointer đến EpollEventLoop
  - `frame_processor`: Callback function để xử lý frames nhận được
  - Returns: `true` nếu thành công, `false` nếu thất bại

- **`void deinit()`**
  - Dọn dẹp resources và đóng socket

- **`bool send_can_frame(const can_frame& frame)`**
  - Gửi CAN frame
  - Returns: `true` nếu thành công, `false` nếu thất bại

#### FrameProcessor Type

```cpp
using FrameProcessor = std::function<void(const can_frame&)>;
```

### EpollEventLoop Class

#### Methods

- **`bool register_event(EvtId* p_evt, int fd, uint32_t events, const Callback& callback)`**
  - Đăng ký event với epoll
  - `p_evt`: Pointer để lưu event ID
  - `fd`: File descriptor
  - `events`: Event flags (EPOLLIN, EPOLLOUT, etc.)
  - `callback`: Callback function khi event xảy ra

- **`bool deregister_event(EvtId evt)`**
  - Hủy đăng ký event

- **`bool run_until_empty()`**
  - Chạy event loop cho đến khi không còn events

### EpollEvent Class

#### Methods

- **`bool init(EpollEventLoop* event_loop, const Callback& callback)`**
  - Khởi tạo event wrapper

- **`void deinit()`**
  - Dọn dẹp event

- **`bool set()`**
  - Trigger event

## Ví dụ ứng dụng thực tế

### 1. CAN Gateway

```cpp
class CanGateway {
private:
    EpollEventLoop event_loop_;
    SocketCanIntf can1_;
    SocketCanIntf can2_;
    
public:
    bool init(const std::string& interface1, const std::string& interface2) {
        // Khởi tạo CAN1 với callback forward sang CAN2
        can1_.init(interface1, &event_loop_, 
                   [this](const can_frame& frame) {
                       can2_.send_can_frame(frame);
                   });
        
        // Khởi tạo CAN2 với callback forward sang CAN1
        can2_.init(interface2, &event_loop_,
                   [this](const can_frame& frame) {
                       can1_.send_can_frame(frame);
                   });
        
        return true;
    }
    
    void run() {
        event_loop_.run_until_empty();
    }
};
```

### 2. CAN Logger

```cpp
class CanLogger {
private:
    SocketCanIntf can_socket_;
    std::ofstream log_file_;
    
public:
    bool init(const std::string& interface, const std::string& log_filename) {
        log_file_.open(log_filename);
        
        auto logger_callback = [this](const can_frame& frame) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            log_file_ << timestamp << ",0x" << std::hex << frame.can_id 
                      << "," << (int)frame.can_dlc;
            
            for (int i = 0; i < frame.can_dlc; i++) {
                log_file_ << ",0x" << std::hex << (int)frame.data[i];
            }
            log_file_ << std::endl;
        };
        
        return can_socket_.init(interface, &event_loop_, logger_callback);
    }
};
```

## Troubleshooting

### Lỗi thường gặp

1. **"Failed to initialize CAN interface"**
   - Kiểm tra CAN interface có tồn tại không: `ip link show`
   - Đảm bảo interface đã được enable: `sudo ip link set up can0`

2. **"Permission denied"**
   - Chạy với quyền sudo hoặc thêm user vào group `dialout`
   - `sudo usermod -a -G dialout $USER`

3. **"No such device"**
   - Load CAN modules: `sudo modprobe can can_raw`
   - Tạo virtual interface nếu cần: `sudo ip link add dev vcan0 type vcan`

### Debug tips

```bash
# Kiểm tra CAN modules
lsmod | grep can

# Kiểm tra CAN interfaces
ip link show | grep can

# Monitor CAN traffic với candump
candump can0

# Test gửi frame
cansend can0 123#DEADBEEF
```

## Performance Tips

1. **Sử dụng non-blocking mode**: Thư viện đã được thiết kế để sử dụng non-blocking sockets
2. **Batch processing**: Xử lý nhiều frames cùng lúc trong callback
3. **Minimize allocations**: Tránh tạo objects mới trong frame callback
4. **Use epoll efficiently**: Event loop đã được tối ưu cho hiệu suất cao

## License

Thư viện này được phát hành dưới license MIT. Xem file LICENSE để biết thêm chi tiết. 