#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <atomic>
#include <vector>
#include <random>

// Global flag để control program
std::atomic<bool> g_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n=== Received SIGINT, stopping... ===" << std::endl;
    g_running = false;
  }
}

class CanReadWriteTest {
private:
  std::unique_ptr<EpollEventLoop> event_loop_reader_;
  //   std::unique_ptr<EpollEventLoop> event_loop_writer_;
  SocketCanIntf                   reader_socket_;
  //   SocketCanIntf                   writer_socket_;

  std::atomic<int>                      received_count_;
  std::atomic<int>                      sent_count_;
  std::chrono::steady_clock::time_point start_time_;

  // Random number generator cho test data
  std::random_device              rd_;
  std::mt19937                    gen_;
  std::uniform_int_distribution<> id_dist_;
  std::uniform_int_distribution<> data_dist_;

public:
  CanReadWriteTest()
    : received_count_(0),
      sent_count_(0),
      gen_(rd_()),
      id_dist_(0x100, 0x7FF),
      data_dist_(0, 255) {
    start_time_ = std::chrono::steady_clock::now();
  }

  bool init(const std::string& interface) {
    std::cout << "=== Initializing Read/Write Test on "
              << interface << " ===" << std::endl;

    // Khởi tạo reader
    event_loop_reader_ = std::make_unique<EpollEventLoop>();
    bool reader_ok     = reader_socket_.init(interface,
                                         event_loop_reader_.get(),
                                         [this](const can_frame& frame) {
                                           log_received_frame(frame);
                                         });

    if (!reader_ok) {
      std::cerr << "❌ Failed to initialize reader socket" << std::endl;
      return false;
    }

    // Khởi tạo writer (với dummy callback)
    // event_loop_writer_ = std::make_unique<EpollEventLoop>();
    // bool writer_ok     = writer_socket_.init(interface,
    //                                      event_loop_writer_.get(),
    //                                      [](const can_frame& frame) {
    //                                        // Dummy callback for writer -
    //                                        không
    //                                        // cần xử lý
    //                                      });

    // if (!writer_ok) {
    //   std::cerr << "❌ Failed to initialize writer socket" << std::endl;
    //   return false;
    // }

    std::cout << "✅ Both reader and writer initialized successfully"
              << std::endl;
    return true;
  }

  void log_received_frame(const can_frame& frame) {
    received_count_++;

    auto now = std::chrono::steady_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);

    // Format timestamp với milliseconds
    std::cout << "[+" << std::setw(8) << duration.count() << "ms] ";

    // Direction indicator
    std::cout << "📥 RX ";

    // Frame counter
    std::cout << "#" << std::setw(4) << received_count_.load() << " ";

    // CAN ID
    if (frame.can_id & CAN_EFF_FLAG) {
      std::cout << "ID:0x" << std::hex << std::setfill('0') << std::setw(8)
                << (frame.can_id & CAN_EFF_MASK) << "(EXT)";
    } else {
      std::cout << "ID:0x" << std::hex << std::setfill('0') << std::setw(3)
                << (frame.can_id & CAN_SFF_MASK) << "(STD)";
    }

    // RTR flag
    if (frame.can_id & CAN_RTR_FLAG) {
      std::cout << " RTR";
    }

    // DLC
    std::cout << " DLC:" << std::dec << (int)frame.can_dlc;

    // Data
    if (frame.can_dlc > 0 && !(frame.can_id & CAN_RTR_FLAG)) {
      std::cout << " Data:[";
      for (int i = 0; i < frame.can_dlc; i++) {
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2)
                  << (int)frame.data[i];
        if (i < frame.can_dlc - 1)
          std::cout << " ";
      }
      std::cout << "]";

      // ASCII view
      std::cout << " \"";
      for (int i = 0; i < frame.can_dlc; i++) {
        char c = frame.data[i];
        std::cout << ((c >= 32 && c <= 126) ? c : '.');
      }
      std::cout << "\"";
    }

    std::cout << std::endl;
  }

  bool send_test_frame(uint32_t                    id,
                       const std::vector<uint8_t>& data,
                       bool                        extended = false,
                       bool                        rtr      = false) {
    can_frame frame = {};
    frame.can_id    = id;

    if (extended) {
      frame.can_id |= CAN_EFF_FLAG;
    }

    if (rtr) {
      frame.can_id |= CAN_RTR_FLAG;
      frame.can_dlc = 0;
    } else {
      frame.can_dlc = std::min((size_t)8, data.size());
      for (size_t i = 0; i < frame.can_dlc; i++) {
        frame.data[i] = data[i];
      }
    }

    bool success = reader_socket_.send_can_frame(frame);
    if (success) {
      sent_count_++;

      auto now      = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_);

      std::cout << "[+" << std::setw(8) << duration.count() << "ms] ";
      std::cout << "📤 TX ";
      std::cout << "#" << std::setw(4) << sent_count_.load() << " ";

      if (extended) {
        std::cout << "ID:0x" << std::hex << std::setfill('0') << std::setw(8)
                  << (id & CAN_EFF_MASK) << "(EXT)";
      } else {
        std::cout << "ID:0x" << std::hex << std::setfill('0') << std::setw(3)
                  << (id & CAN_SFF_MASK) << "(STD)";
      }

      if (rtr) {
        std::cout << " RTR";
      }

      std::cout << " DLC:" << std::dec << (int)frame.can_dlc;

      if (!rtr && frame.can_dlc > 0) {
        std::cout << " Data:[";
        for (int i = 0; i < frame.can_dlc; i++) {
          std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2)
                    << (int)frame.data[i];
          if (i < frame.can_dlc - 1)
            std::cout << " ";
        }
        std::cout << "]";
      }

      std::cout << std::endl;
    }

    return success;
  }

  void send_random_frame() {
    uint32_t             id = id_dist_(gen_);
    std::vector<uint8_t> data;

    // Random data length 1-8
    int dlc = (gen_() % 8) + 1;
    for (int i = 0; i < dlc; i++) {
      data.push_back(data_dist_(gen_));
    }

    send_test_frame(id, data);
  }

  void send_sequence_frames() {
    // Gửi sequence frames với pattern đặc biệt
    for (int i = 0; i < 5; i++) {
      std::vector<uint8_t> data = {(uint8_t)(0x10 + i),
                                   (uint8_t)(0x20 + i),
                                   (uint8_t)(0x30 + i),
                                   (uint8_t)(0x40 + i)};
      send_test_frame(0x300 + i, data);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

  void run_test() {
    std::cout << "\n🚀 Starting Read/Write Test..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Thread để đọc frames - polling approach
    std::thread reader_thread([this]() {
      while (g_running.load()) {
        reader_socket_.read_nonblocking();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    // Main thread gửi test frames theo interval
    auto last_random   = std::chrono::steady_clock::now();
    auto last_sequence = std::chrono::steady_clock::now();

    while (g_running.load()) {
      auto now = std::chrono::steady_clock::now();

      // Gửi random frame mỗi 2 giây
      if (std::chrono::duration_cast<std::chrono::seconds>(now - last_random)
            .count() >= 2) {
        send_random_frame();
        last_random = now;
      }

      // Gửi sequence frames mỗi 10 giây
      if (std::chrono::duration_cast<std::chrono::seconds>(now - last_sequence)
            .count() >= 10) {
        std::cout << "\n--- Sending sequence frames ---" << std::endl;
        send_sequence_frames();
        std::cout << "--- Sequence completed ---\n" << std::endl;
        last_sequence = now;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n=== Stopping threads... ===" << std::endl;

    if (reader_thread.joinable()) {
      reader_thread.join();
    }

    cleanup();
    print_statistics();
  }

  void cleanup() {
    reader_socket_.deinit();
    // writer_socket_.deinit();
  }

  void print_statistics() {
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration =
      std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time_);

    std::cout << "\n=== Test Statistics ===" << std::endl;
    std::cout << "Total runtime: " << total_duration.count() << " seconds"
              << std::endl;
    std::cout << "Frames sent: " << sent_count_.load() << std::endl;
    std::cout << "Frames received: " << received_count_.load() << std::endl;

    if (total_duration.count() > 0) {
      double sent_rate = (double)sent_count_.load() / total_duration.count();
      double recv_rate =
        (double)received_count_.load() / total_duration.count();
      std::cout << "Send rate: " << std::fixed << std::setprecision(2)
                << sent_rate << " frames/sec" << std::endl;
      std::cout << "Receive rate: " << std::fixed << std::setprecision(2)
                << recv_rate << " frames/sec" << std::endl;
    }

    std::cout << "✅ Test completed successfully" << std::endl;
  }

  ~CanReadWriteTest() {
    cleanup();
  }
};

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [interface]" << std::endl;
  std::cout << "  interface: CAN interface name (default: vcan0)" << std::endl;
  std::cout << "\nThis program will:" << std::endl;
  std::cout << "  - Monitor CAN frames on the interface" << std::endl;
  std::cout << "  - Send test frames automatically" << std::endl;
  std::cout << "  - Log all activity with timestamps" << std::endl;
  std::cout << "\nSetup virtual CAN interface:" << std::endl;
  std::cout << "  sudo modprobe vcan" << std::endl;
  std::cout << "  sudo ip link add dev vcan0 type vcan" << std::endl;
  std::cout << "  sudo ip link set up vcan0" << std::endl;
}

int main(int argc, char* argv[]) {
  // Setup signal handler
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  std::string interface = "vcan0";  // Default

  if (argc > 1) {
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
      print_usage(argv[0]);
      return 0;
    }
    interface = argv[1];
  }

  std::cout << "=== CAN Read/Write Test ===" << std::endl;
  std::cout << "Interface: " << interface << std::endl;

  CanReadWriteTest test;

  if (!test.init(interface)) {
    std::cerr << "❌ Failed to initialize test" << std::endl;
    std::cerr << "Make sure the interface exists: ip link show "
              << interface << std::endl;
    return 1;
  }

  test.run_test();

  return 0;
}