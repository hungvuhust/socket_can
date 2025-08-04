#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

// Integration test mô phỏng việc sử dụng thực tế thư viện
class CanTestApp {
public:
  CanTestApp() : frame_count_(0), running_(false) {
  }

  ~CanTestApp() {
    if (event_loop_) {
      socket_can_.deinit();
    }
  }

  bool init(const std::string& interface) {
    // Khởi tạo event loop
    event_loop_ = std::make_unique<EpollEventLoop>();

    // Khởi tạo socket CAN với callback
    bool success = socket_can_.init(interface,
                                    event_loop_.get(),
                                    [this](const can_frame& frame) {
                                      handle_received_frame(frame);
                                    });

    if (!success) {
      std::cout
        << "Note: CAN interface '"
        << interface << "' không khả dụng. Đây là test integration nên OK."
        << std::endl;
      return false;
    }

    return true;
  }

  void handle_received_frame(const can_frame& frame) {
    frame_count_++;
    std::cout << "Received frame #" << frame_count_ << " - ID: 0x" << std::hex
              << (frame.can_id & CAN_EFF_MASK) << ", DLC: " << std::dec
              << (int)frame.can_dlc << std::endl;
  }

  bool send_test_frame(uint32_t id, const std::vector<uint8_t>& data) {
    can_frame frame = {};
    frame.can_id    = id;
    frame.can_dlc   = std::min((size_t)8, data.size());

    for (size_t i = 0; i < frame.can_dlc; i++) {
      frame.data[i] = data[i];
    }

    return socket_can_.send_can_frame(frame);
  }

  void run_for_duration(std::chrono::milliseconds duration) {
    running_ = true;

    // Không chạy event loop vô tận - chỉ test trong thời gian ngắn
    std::this_thread::sleep_for(duration);
    running_ = false;

    // Dọn dẹp socket trước
    socket_can_.deinit();
  }

  int get_frame_count() const {
    return frame_count_;
  }

private:
  std::unique_ptr<EpollEventLoop> event_loop_;
  SocketCanIntf                   socket_can_;
  std::atomic<int>                frame_count_;
  std::atomic<bool>               running_;
};

void test_event_loop_performance() {
  std::cout << "\n=== Test Event Loop Performance ===" << std::endl;

  EpollEventLoop                     loop;
  const int                          num_events = 100;
  std::atomic<int>                   triggered_count(0);
  std::vector<EpollEventLoop::EvtId> evt_ids;
  std::vector<int>                   pipe_fds;

  // Tạo nhiều pipe events
  for (int i = 0; i < num_events; i++) {
    int pipefd[2];
    if (pipe(pipefd) == -1)
      break;

    EpollEventLoop::EvtId evt_id;
    bool                  success = loop.register_event(&evt_id,
                                       pipefd[0],
                                       EPOLLIN,
                                       [&triggered_count, i](uint32_t mask) {
                                         triggered_count++;
                                         // std::cout << "Event " << i << "
                                         // triggered" << std::endl;
                                       });

    if (success) {
      evt_ids.push_back(evt_id);
      pipe_fds.push_back(pipefd[0]);
      pipe_fds.push_back(pipefd[1]);
    }
  }

  std::cout << "Registered " << evt_ids.size() << " events" << std::endl;

  // Trigger một số events
  for (size_t i = 0; i < evt_ids.size() && i < 10; i++) {
    char data = 'x';
    write(pipe_fds[i * 2 + 1], &data, 1);
  }

  // Cleanup
  for (auto evt_id : evt_ids) {
    loop.deregister_event(evt_id);
  }

  for (int fd : pipe_fds) {
    close(fd);
  }

  std::cout << "Events triggered: " << triggered_count.load() << std::endl;
}

int main() {
  std::cout << "=== SocketCAN Integration Test ===" << std::endl;

  // Test 1: Event loop performance
  test_event_loop_performance();

  // Test 2: CanTestApp với virtual interface (sẽ fail nhưng OK)
  std::cout << "\n=== Test CAN Interface (Virtual) ===" << std::endl;
  CanTestApp app;

  // Thử các interface phổ biến (có thể không tồn tại)
  std::vector<std::string> test_interfaces = {"vcan0", "can0", "eth0"};

  bool initialized = false;
  for (const auto& interface : test_interfaces) {
    std::cout << "Trying interface: " << interface << std::endl;
    if (app.init(interface)) {
      std::cout << "Success with interface: " << interface << std::endl;
      initialized = true;
      break;
    }
  }

  if (initialized) {
    // Test gửi frame
    std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04};
    app.send_test_frame(0x123, test_data);

    // Chạy trong thời gian ngắn
    app.run_for_duration(std::chrono::milliseconds(100));

    std::cout << "Total frames received: " << app.get_frame_count()
              << std::endl;
  } else {
    std::cout
      << "No CAN interface available - this is expected in test environment"
      << std::endl;
  }

  // Test 3: Frame format validation
  std::cout << "\n=== Test Frame Formats ===" << std::endl;

  // Standard frame
  can_frame std_frame = {};
  std_frame.can_id    = 0x7FF;  // Max standard ID
  std_frame.can_dlc   = 8;
  std::cout << "Standard frame ID mask: 0x" << std::hex
            << (std_frame.can_id & CAN_SFF_MASK) << std::endl;

  // Extended frame
  can_frame ext_frame = {};
  ext_frame.can_id    = 0x1FFFFFFF | CAN_EFF_FLAG;  // Max extended ID
  ext_frame.can_dlc   = 8;
  std::cout << "Extended frame ID mask: 0x" << std::hex
            << (ext_frame.can_id & CAN_EFF_MASK) << std::endl;

  // RTR frame
  can_frame rtr_frame = {};
  rtr_frame.can_id    = 0x123 | CAN_RTR_FLAG;
  rtr_frame.can_dlc   = 0;  // RTR frames have no data
  std::cout << "RTR frame: " << (rtr_frame.can_id & CAN_RTR_FLAG ? "Yes" : "No")
            << std::endl;

  std::cout << "\n=== Integration Test Completed ===" << std::endl;

  return 0;
}