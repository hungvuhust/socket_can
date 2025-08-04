#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

class CanSender {
private:
  EpollEventLoop event_loop_;
  SocketCanIntf  socket_can_;
  bool           initialized_;

public:
  CanSender() : initialized_(false) {
  }

  bool init(const std::string& interface) {
    // Dummy callback vì chỉ cần send
    auto dummy_callback = [](const can_frame& frame) {
      // Không làm gì, chỉ send
    };

    initialized_ = socket_can_.init(interface, &event_loop_, dummy_callback);
    return initialized_;
  }

  bool send_frame(uint32_t                    id,
                  const std::vector<uint8_t>& data,
                  bool                        extended = false,
                  bool                        rtr      = false) {
    if (!initialized_) {
      std::cerr << "❌ Sender not initialized!" << std::endl;
      return false;
    }

    can_frame frame = {};
    frame.can_id    = id;

    if (extended) {
      frame.can_id |= CAN_EFF_FLAG;
    }

    if (rtr) {
      frame.can_id |= CAN_RTR_FLAG;
      frame.can_dlc = 0;  // RTR frames have no data
    } else {
      frame.can_dlc = std::min((size_t)8, data.size());
      for (size_t i = 0; i < frame.can_dlc; i++) {
        frame.data[i] = data[i];
      }
    }

    std::cout << "📤 Sending frame - ID: 0x" << std::hex << std::uppercase
              << (id & (extended ? CAN_EFF_MASK : CAN_SFF_MASK));
    if (extended)
      std::cout << " (Extended)";
    if (rtr)
      std::cout << " (RTR)";
    std::cout << ", DLC: " << std::dec << (int)frame.can_dlc;

    if (!rtr && frame.can_dlc > 0) {
      std::cout << ", Data: ";
      for (int i = 0; i < frame.can_dlc; i++) {
        std::cout << "0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(2) << (int)frame.data[i];
        if (i < frame.can_dlc - 1)
          std::cout << " ";
      }
    }
    std::cout << std::endl;

    return socket_can_.send_can_frame(frame);
  }

  void deinit() {
    if (initialized_) {
      socket_can_.deinit();
      initialized_ = false;
    }
  }

  ~CanSender() {
    deinit();
  }
};

void demo_standard_frames(CanSender& sender) {
  std::cout << "\n=== Demo: Standard Frames ===" << std::endl;

  // Test với data khác nhau
  sender.send_frame(0x123, {0xDE, 0xAD, 0xBE, 0xEF});
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  sender.send_frame(0x456, {0x48, 0x65, 0x6C, 0x6C, 0x6F});  // "Hello"
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  sender.send_frame(0x789, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demo_extended_frames(CanSender& sender) {
  std::cout << "\n=== Demo: Extended Frames ===" << std::endl;

  sender.send_frame(0x12345678, {0xAA, 0xBB, 0xCC, 0xDD}, true);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  sender.send_frame(0x1ABCDEF0, {0x11, 0x22, 0x33}, true);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demo_rtr_frames(CanSender& sender) {
  std::cout << "\n=== Demo: RTR Frames ===" << std::endl;

  sender.send_frame(0x100, {}, false, true);  // Standard RTR
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  sender.send_frame(0x10000001, {}, true, true);  // Extended RTR
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demo_continuous_data(CanSender& sender) {
  std::cout << "\n=== Demo: Continuous Data Stream ===" << std::endl;

  for (int i = 0; i < 10; i++) {
    std::vector<uint8_t> data = {(uint8_t)(i & 0xFF),
                                 (uint8_t)((i >> 8) & 0xFF),
                                 (uint8_t)(0x50 + i),
                                 (uint8_t)(0xA0 + i)};

    sender.send_frame(0x200 + i, data);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main() {
  std::cout << "=== CAN Frame Sender Test (vcan0) ===" << std::endl;
  std::cout << "This will send various CAN frames to vcan0" << std::endl;
  std::cout << "Run can_reader_test in another terminal to see the frames"
            << std::endl;
  std::cout << std::string(60, '=') << std::endl;

  CanSender sender;

  // Khởi tạo với vcan0
  std::cout << "Initializing CAN sender on vcan0..." << std::endl;
  if (!sender.init("vcan0")) {
    std::cerr << "❌ Failed to initialize vcan0!" << std::endl;
    std::cerr << "Make sure virtual CAN interface is up:" << std::endl;
    std::cerr << "  sudo modprobe vcan" << std::endl;
    std::cerr << "  sudo ip link add dev vcan0 type vcan" << std::endl;
    std::cerr << "  sudo ip link set up vcan0" << std::endl;
    return 1;
  }

  std::cout << "✅ Successfully connected to vcan0" << std::endl;
  std::cout << "🚀 Starting to send test frames..." << std::endl;

  // Demo các loại frames khác nhau
  demo_standard_frames(sender);
  demo_extended_frames(sender);
  demo_rtr_frames(sender);
  demo_continuous_data(sender);

  std::cout << "\n=== Sending completed ===" << std::endl;
  std::cout << "✅ All test frames sent successfully" << std::endl;

  return 0;
}