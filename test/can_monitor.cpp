#include "socket_can/socket_can.hpp"
#include "socket_can/epoll_event_loop.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <signal.h>

// Global flag để stop monitoring
volatile bool running = true;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", stopping monitor..." << std::endl;
    running = false;
}

class CanMonitor {
public:
    CanMonitor() : frame_count_(0) {}
    
    bool init(const std::string& interface) {
        std::cout << "Initializing CAN monitor on interface: " << interface << std::endl;
        
        // Khởi tạo event loop
        event_loop_ = std::make_unique<EpollEventLoop>();
        
        // Khởi tạo socket CAN với callback
        bool success = socket_can_.init(interface, event_loop_.get(), 
                                        [this](const can_frame& frame) {
                                            log_received_frame(frame);
                                        });
        
        if (!success) {
            std::cerr << "Failed to initialize CAN interface: " << interface << std::endl;
            return false;
        }
        
        std::cout << "CAN monitor initialized successfully on " << interface << std::endl;
        return true;
    }
    
    void log_received_frame(const can_frame& frame) {
        frame_count_++;
        
        // Get current time for timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        // Format timestamp
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        std::cout << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        
        // Frame counter
        std::cout << "Frame #" << std::setw(6) << frame_count_ << " - ";
        
        // CAN ID với format phù hợp
        if (frame.can_id & CAN_EFF_FLAG) {
            // Extended frame
            std::cout << "ID: 0x" << std::hex << std::setfill('0') << std::setw(8) 
                      << (frame.can_id & CAN_EFF_MASK) << " (EXT)";
        } else {
            // Standard frame  
            std::cout << "ID: 0x" << std::hex << std::setfill('0') << std::setw(3)
                      << (frame.can_id & CAN_SFF_MASK) << " (STD)";
        }
        
        // RTR flag
        if (frame.can_id & CAN_RTR_FLAG) {
            std::cout << " RTR";
        }
        
        // Data length
        std::cout << " DLC: " << std::dec << (int)frame.can_dlc;
        
        // Data bytes
        if (frame.can_dlc > 0 && !(frame.can_id & CAN_RTR_FLAG)) {
            std::cout << " Data: [";
            for (int i = 0; i < frame.can_dlc; i++) {
                std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) 
                          << (int)frame.data[i];
                if (i < frame.can_dlc - 1) std::cout << " ";
            }
            std::cout << "]";
            
            // ASCII representation nếu có thể đọc được
            std::cout << " ASCII: \"";
            for (int i = 0; i < frame.can_dlc; i++) {
                char c = frame.data[i];
                if (c >= 32 && c <= 126) {  // Printable ASCII
                    std::cout << c;
                } else {
                    std::cout << ".";
                }
            }
            std::cout << "\"";
        }
        
        std::cout << std::endl;
        std::cout.flush();  // Force flush để hiện ngay
    }
    
    void start_monitoring() {
        std::cout << "\n=== Starting CAN Monitor ===" << std::endl;
        std::cout << "Press Ctrl+C to stop monitoring\n" << std::endl;
        
        // Chạy event loop trong thread riêng
        std::thread monitor_thread([this]() {
            while (running && event_loop_) {
                // Sử dụng epoll_wait với timeout để có thể check running flag
                // Tạm thời dùng polling approach đơn giản
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Main thread chờ signal hoặc running flag
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "\nStopping monitor..." << std::endl;
        
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        
        socket_can_.deinit();
        
        std::cout << "\nMonitoring stopped. Total frames received: " << frame_count_ << std::endl;
    }
    
private:
    std::unique_ptr<EpollEventLoop> event_loop_;
    SocketCanIntf socket_can_;
    int frame_count_;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [interface]" << std::endl;
    std::cout << "  interface: CAN interface name (default: vcan0)" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << program_name << " vcan0" << std::endl;
    std::cout << "  " << program_name << " can0" << std::endl;
    std::cout << "\nTo test with virtual CAN:" << std::endl;
    std::cout << "  sudo modprobe vcan" << std::endl;
    std::cout << "  sudo ip link add dev vcan0 type vcan" << std::endl;
    std::cout << "  sudo ip link set up vcan0" << std::endl;
    std::cout << "\nTo send test frames:" << std::endl;
    std::cout << "  cansend vcan0 123#DEADBEEF" << std::endl;
    std::cout << "  cansend vcan0 456#01020304" << std::endl;
}

int main(int argc, char* argv[]) {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::string interface = "vcan0";  // Default interface
    
    // Parse command line arguments
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        interface = argv[1];
    }
    
    std::cout << "=== CAN Frame Monitor ===" << std::endl;
    std::cout << "Interface: " << interface << std::endl;
    
    CanMonitor monitor;
    
    if (!monitor.init(interface)) {
        std::cerr << "Failed to initialize CAN monitor" << std::endl;
        std::cerr << "\nTip: Make sure the CAN interface exists:" << std::endl;
        std::cerr << "  ip link show " << interface << std::endl;
        return 1;
    }
    
    monitor.start_monitoring();
    
    return 0;
}