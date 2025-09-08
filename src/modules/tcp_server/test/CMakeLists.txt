# TCP Server Tests
cmake_minimum_required(VERSION 3.14)

# Create test executable
add_executable(tcp_server_test
    tcp_server_test.cpp
)

# Link against tcp_server module and gtest
target_link_libraries(tcp_server_test
    tcp_server
    ${GTEST_LIB_FILES}
)

# Set C++ standard
target_compile_features(tcp_server_test PUBLIC cxx_std_17)

# Add test to CTest
add_test(NAME tcp_server_test COMMAND tcp_server_test)