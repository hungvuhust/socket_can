#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <atomic>

// Global flag để stop program với Ctrl+C
std::atomic<bool> g_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n=== Received SIGINT, stopping... ===" << std::endl;
    g_running = false;
  }
}

class CanFrameLogger {
private:
  std::atomic<int>                      frame_count_;
  std::chrono::steady_clock::time_point start_time_;

public:
  CanFrameLogger() : frame_count_(0) {
    start_time_ = std::chrono::steady_clock::now();
  }

  void operator()(const can_frame& frame) {
    frame_count_++;
    auto now = std::chrono::steady_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);

    // Header với timestamp
    std::cout << "\n=== Frame #" << frame_count_ << " (+" << duration.count()
              << "ms) ===" << std::endl;

    // CAN ID analysis
    uint32_t raw_id   = frame.can_id;
    uint32_t clean_id = raw_id & CAN_EFF_MASK;

    std::cout << "Raw CAN ID: 0x" << std::hex << std::uppercase
              << std::setfill('0') << std::setw(8) << raw_id << std::endl;

    if (raw_id & CAN_EFF_FLAG) {
      std::cout << "Type: Extended Frame (29-bit ID)" << std::endl;
      std::cout << "Clean ID: 0x" << std::hex << std::uppercase
                << std::setfill('0') << std::setw(8) << clean_id << std::endl;
    } else {
      std::cout << "Type: Standard Frame (11-bit ID)" << std::endl;
      std::cout << "Clean ID: 0x" << std::hex << std::uppercase
                << std::setfill('0') << std::setw(3)
                << (clean_id & CAN_SFF_MASK) << std::endl;
    }

    if (raw_id & CAN_RTR_FLAG) {
      std::cout << "RTR: Remote Transmission Request" << std::endl;
    }

    if (raw_id & CAN_ERR_FLAG) {
      std::cout << "ERR: Error Frame" << std::endl;
    }

    // Data Length Code
    std::cout << "DLC: " << std::dec << (int)frame.can_dlc << " bytes"
              << std::endl;

    // Data bytes
    if (frame.can_dlc > 0 && !(raw_id & CAN_RTR_FLAG)) {
      std::cout << "Data (HEX): ";
      for (int i = 0; i < frame.can_dlc; i++) {
        std::cout << "0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(2) << (int)frame.data[i];
        if (i < frame.can_dlc - 1)
          std::cout << " ";
      }
      std::cout << std::endl;

      std::cout << "Data (DEC): ";
      for (int i = 0; i < frame.can_dlc; i++) {
        std::cout << std::dec << (int)frame.data[i];
        if (i < frame.can_dlc - 1)
          std::cout << " ";
      }
      std::cout << std::endl;

      std::cout << "Data (CHR): ";
      for (int i = 0; i < frame.can_dlc; i++) {
        char c = frame.data[i];
        if (c >= 32 && c <= 126) {  // Printable ASCII
          std::cout << "'" << c << "'";
        } else {
          std::cout << "'.'";
        }
        if (i < frame.can_dlc - 1)
          std::cout << " ";
      }
      std::cout << std::endl;
    }

    std::cout << "Total frames received: " << frame_count_.load() << std::endl;
    std::cout << std::string(50, '-') << std::endl;
  }

  int get_frame_count() const {
    return frame_count_.load();
  }
};

int main() {
  std::cout << "=== CAN Frame Reader Test (vcan0) ===" << std::endl;
  std::cout << "Press Ctrl+C to stop listening..." << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Setup signal handler
  signal(SIGINT, signal_handler);

  // Khởi tạo components
  EpollEventLoop event_loop;
  CanFrameLogger frame_logger;
  SocketCanIntf  socket_can;

  // Khởi tạo với vcan0
  std::cout << "Initializing CAN interface: vcan0..." << std::endl;
  bool success = socket_can.init("vcan0", &event_loop, std::ref(frame_logger));

  if (!success) {
    std::cerr << "❌ Failed to initialize vcan0!" << std::endl;
    std::cerr << "Make sure virtual CAN interface is up:" << std::endl;
    std::cerr << "  sudo modprobe vcan" << std::endl;
    std::cerr << "  sudo ip link add dev vcan0 type vcan" << std::endl;
    std::cerr << "  sudo ip link set up vcan0" << std::endl;
    std::cerr << "\nTest sending with:" << std::endl;
    std::cerr << "  cansend vcan0 123#DEADBEEF" << std::endl;
    return 1;
  }

  std::cout << "✅ Successfully connected to vcan0" << std::endl;
  std::cout << "🎧 Listening for CAN frames..." << std::endl;
  std::cout << "\nTo test, run in another terminal:" << std::endl;
  std::cout << "  cansend vcan0 123#DEADBEEF" << std::endl;
  std::cout << "  cansend vcan0 456#48656C6C6F" << std::endl;
  std::cout << "  cansend vcan0 789#01020304050607" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Event loop với timeout để có thể check g_running
  while (g_running.load()) {
    // Đọc non-blocking để check có frame mới không
    socket_can.read_nonblocking();

    // Sleep ngắn để không waste CPU
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::cout << "\n=== Shutting down ===" << std::endl;
  std::cout << "Total frames processed: " << frame_logger.get_frame_count()
            << std::endl;

  socket_can.deinit();

  std::cout << "✅ Clean shutdown completed" << std::endl;
  return 0;
}