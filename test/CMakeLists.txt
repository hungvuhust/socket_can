cmake_minimum_required(VERSION 3.10.0)

# Unit tests
add_executable(test_socket_can
    test_socket_can.cpp
)

# Integration tests  
add_executable(integration_test
    integration_test.cpp
)

# CAN reader test (real-time frame logging)
add_executable(can_reader_test
    can_reader_test.cpp
)

# CAN sender test (frame generation)
add_executable(can_sender_test
    can_sender_test.cpp
)

# CAN monitor (frame monitoring)
add_executable(can_monitor
    can_monitor.cpp
)

# CAN read/write test (combined test)
add_executable(can_read_write_test
    can_read_write_test.cpp
)

# Link với thư viện SocketCAN
target_link_libraries(test_socket_can 
    SocketCAN
)

target_link_libraries(integration_test
    SocketCAN
)

target_link_libraries(can_reader_test
    SocketCAN
)

target_link_libraries(can_sender_test
    SocketCAN
)

target_link_libraries(can_monitor
    SocketCAN
)

target_link_libraries(can_read_write_test
    SocketCAN
)

# Include directories
target_include_directories(test_socket_can PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_include_directories(integration_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_include_directories(can_reader_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_include_directories(can_sender_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_include_directories(can_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_include_directories(can_read_write_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Enable testing
enable_testing()
add_test(NAME socket_can_tests COMMAND test_socket_can)
add_test(NAME integration_tests COMMAND integration_test)

# Compiler flags
target_compile_features(test_socket_can PRIVATE cxx_std_17)
target_compile_features(integration_test PRIVATE cxx_std_17)
target_compile_features(can_reader_test PRIVATE cxx_std_17)
target_compile_features(can_sender_test PRIVATE cxx_std_17)
target_compile_features(can_monitor PRIVATE cxx_std_17)
target_compile_features(can_read_write_test PRIVATE cxx_std_17)

set_target_properties(test_socket_can PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(integration_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(can_reader_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(can_sender_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(can_monitor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(can_read_write_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)