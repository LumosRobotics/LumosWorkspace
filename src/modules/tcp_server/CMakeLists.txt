# TCP Server Module
cmake_minimum_required(VERSION 3.14)

# Create a static library for the TCP server
add_library(tcp_server STATIC
    tcp_server.cpp
    tcp_server.h
)

# Set include directories for the library
target_include_directories(tcp_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link required system libraries
target_link_libraries(tcp_server
    pthread
)

# Set C++ standard
target_compile_features(tcp_server PUBLIC cxx_std_17)

# Add tests subdirectory
add_subdirectory(test)