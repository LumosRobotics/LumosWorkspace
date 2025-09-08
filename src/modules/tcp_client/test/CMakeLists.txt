# TCP Client Tests
cmake_minimum_required(VERSION 3.14)

# Create test executable
add_executable(tcp_client_test
    tcp_client_test.cpp
)

# Link against tcp_client module, tcp_server module (for integration tests), and gtest
target_link_libraries(tcp_client_test
    tcp_client
    tcp_server
    ${GTEST_LIB_FILES}
)

# Set C++ standard
target_compile_features(tcp_client_test PUBLIC cxx_std_17)

# Add test to CTest
add_test(NAME tcp_client_test COMMAND tcp_client_test)