# Minimum CMake version required
cmake_minimum_required(VERSION 3.14)

# Project name and languages
project(LumosWorkspace C CXX)

# Set the C++ standard (adjust according to your needs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally enable debugging support
set(CMAKE_BUILD_TYPE Debug)

# Optionally define a version
set(PROJECT_VERSION "1.0.0")

# Googletest
link_directories(${CMAKE_SOURCE_DIR}/third_party/googletest/lib)
include_directories(${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include)

set(GTEST_LIB_FILES libgtest.a
                    libgtest_main.a
                    pthread)

# Python
include_directories(${CMAKE_SOURCE_DIR}/third_party/cpython/Include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/cpython)
link_directories(${CMAKE_SOURCE_DIR}/third_party/cpython)

# Qt6
set(CMAKE_PREFIX_PATH "/usr/local/opt/qt/lib/cmake")
find_package(Qt6 COMPONENTS Core Widgets QUIET)

# Nlohmann
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann/include)

# Duoplot
# include_directories(${CMAKE_SOURCE_DIR}/third_party/dvs/src/interfaces/cpp)

# include_directories(${CMAKE_SOURCE_DIR}/src/)



add_compile_options(-Wall -Wextra -pedantic)

# Enable testing
enable_testing()

# Add modules
add_subdirectory(src/modules/tcp_server)
add_subdirectory(src/modules/tcp_client)
add_subdirectory(src/modules/settings_handler)

add_subdirectory(src/applications/simple)
add_subdirectory(src/applications/repl)
add_subdirectory(src/applications/simple_transmitter)

# Only add repl_gui if Qt6 is found
if(Qt6_FOUND)
    add_subdirectory(src/applications/repl_gui)
    message(STATUS "Building repl_gui with Qt6")
else()
    message(STATUS "Qt6 not found, skipping repl_gui")
endif()
