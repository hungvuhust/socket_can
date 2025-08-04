cmake_minimum_required(VERSION 3.10.0)
project(SocketCAN VERSION 0.1.0 LANGUAGES C CXX)

add_library(SocketCAN 
    src/socket_can.cpp
    src/epoll_event_loop.cpp
)

target_include_directories(SocketCAN PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable C++17
target_compile_features(SocketCAN PUBLIC cxx_std_17)

# Add subdirectory for tests
add_subdirectory(test)

