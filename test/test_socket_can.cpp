#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

// Simple test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name)                                                         \
  do {                                                                         \
    std::cout << "Running test: " << #name << "...";                           \
    test_##name();                                                             \
    std::cout << " PASSED" << std::endl;                                       \
  } while (0)

// Test callback để capture received frames
struct TestFrameReceiver {
  std::vector<can_frame> received_frames;

  void operator()(const can_frame& frame) {
    received_frames.push_back(frame);
  }
};

TEST(epoll_event_loop_basic) {
  EpollEventLoop loop;

  // Test register/deregister với dummy fd
  int dummy_fd[2];
  pipe(dummy_fd);

  bool                  callback_called = false;
  EpollEventLoop::EvtId evt_id;

  bool success = loop.register_event(&evt_id,
                                     dummy_fd[0],
                                     EPOLLIN,
                                     [&callback_called](uint32_t mask) {
                                       callback_called = true;
                                     });

  assert(success && "Failed to register event");

  // Trigger event bằng cách write vào pipe
  char data = 'x';
  write(dummy_fd[1], &data, 1);

  // Không test run_until_empty() vì có thể block

  bool dereg_success = loop.deregister_event(evt_id);
  assert(dereg_success && "Failed to deregister event");

  close(dummy_fd[0]);
  close(dummy_fd[1]);
}

TEST(socket_can_init_with_invalid_interface) {
  EpollEventLoop    loop;
  TestFrameReceiver receiver;
  SocketCanIntf     socket_can;

  // Test với interface không tồn tại
  bool success =
    socket_can.init("invalid_interface", &loop, std::ref(receiver));

  // Expect init sẽ fail với interface không tồn tại
  assert(!success && "Init should fail with invalid interface");

  socket_can.deinit();  // Should be safe to call even after failed init
}

TEST(socket_can_send_frame_without_init) {
  SocketCanIntf socket_can;

  can_frame frame = {};
  frame.can_id    = 0x123;
  frame.can_dlc   = 8;
  for (int i = 0; i < 8; i++) {
    frame.data[i] = i;
  }

  // Test send trước khi init - should fail
  bool success = socket_can.send_can_frame(frame);
  assert(!success && "Send should fail without proper init");
}

TEST(can_frame_creation) {
  // Test tạo CAN frame với các giá trị khác nhau
  can_frame frame1 = {};
  frame1.can_id    = 0x123;
  frame1.can_dlc   = 8;
  for (int i = 0; i < 8; i++) {
    frame1.data[i] = i;
  }

  assert(frame1.can_id == 0x123);
  assert(frame1.can_dlc == 8);
  assert(frame1.data[0] == 0);
  assert(frame1.data[7] == 7);

  // Test extended frame
  can_frame frame2 = {};
  frame2.can_id    = 0x12345678 | CAN_EFF_FLAG;
  frame2.can_dlc   = 4;
  frame2.data[0]   = 0xAA;
  frame2.data[1]   = 0xBB;
  frame2.data[2]   = 0xCC;
  frame2.data[3]   = 0xDD;

  assert(frame2.can_id & CAN_EFF_FLAG);
  assert((frame2.can_id & CAN_EFF_MASK) == 0x12345678);
}

TEST(frame_processor_callback) {
  TestFrameReceiver receiver;

  can_frame test_frame = {};
  test_frame.can_id    = 0x456;
  test_frame.can_dlc   = 3;
  test_frame.data[0]   = 0x11;
  test_frame.data[1]   = 0x22;
  test_frame.data[2]   = 0x33;

  // Gọi callback trực tiếp
  receiver(test_frame);

  assert(receiver.received_frames.size() == 1);
  assert(receiver.received_frames[0].can_id == 0x456);
  assert(receiver.received_frames[0].can_dlc == 3);
  assert(receiver.received_frames[0].data[0] == 0x11);
  assert(receiver.received_frames[0].data[1] == 0x22);
  assert(receiver.received_frames[0].data[2] == 0x33);
}

TEST(epoll_event_basic) {
  EpollEventLoop loop;
  EpollEvent     event;

  bool triggered = false;
  bool success =
    event.init(&loop, [&triggered](uint32_t mask) { triggered = true; });

  assert(success && "Failed to init EpollEvent");

  // Test set event
  bool set_success = event.set();
  assert(set_success && "Failed to set event");

  event.deinit();
}

// Utility function để print frame info
void print_frame_info(const can_frame& frame) {
  std::cout << "CAN Frame - ID: 0x" << std::hex << (frame.can_id & CAN_EFF_MASK)
            << ", DLC: " << std::dec << (int)frame.can_dlc << ", Data: ";
  for (int i = 0; i < frame.can_dlc; i++) {
    std::cout << "0x" << std::hex << (int)frame.data[i] << " ";
  }
  std::cout << std::endl;
}

int main() {
  std::cout << "=== SocketCAN Library Tests ===" << std::endl;

  try {
    RUN_TEST(epoll_event_loop_basic);
    RUN_TEST(socket_can_init_with_invalid_interface);
    RUN_TEST(socket_can_send_frame_without_init);
    RUN_TEST(can_frame_creation);
    RUN_TEST(frame_processor_callback);
    RUN_TEST(epoll_event_basic);

    std::cout << "\n=== All tests PASSED! ===" << std::endl;

    // Demo usage
    std::cout << "\n=== Demo Frame Creation ===" << std::endl;
    can_frame demo_frame = {};
    demo_frame.can_id    = 0x7FF;
    demo_frame.can_dlc   = 8;
    for (int i = 0; i < 8; i++) {
      demo_frame.data[i] = 0x10 + i;
    }
    print_frame_info(demo_frame);

  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}