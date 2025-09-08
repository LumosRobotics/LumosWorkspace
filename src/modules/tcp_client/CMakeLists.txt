# TCP Client Module
cmake_minimum_required(VERSION 3.14)

# Create a static library for the TCP client
add_library(tcp_client STATIC
    tcp_client.cpp
    tcp_client.h
)

# Set include directories for the library
target_include_directories(tcp_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link required system libraries
target_link_libraries(tcp_client
    pthread
)

# Set C++ standard
target_compile_features(tcp_client PUBLIC cxx_std_17)

# Add tests subdirectory
add_subdirectory(test)