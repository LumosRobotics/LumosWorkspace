# GUI Test Suite
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Only build GUI tests if debug port is enabled
if(NOT ENABLE_DEBUG_PORT)
    message(STATUS "Debug port disabled, skipping GUI tests")
    return()
endif()

# Only build if Qt6 is found (since we're testing the GUI)
if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, skipping GUI tests")
    return()
endif()

# Collect all test source files
file(GLOB TEST_SOURCE_FILES "*.cpp")

# Create the test executable
add_executable(gui_test ${TEST_SOURCE_FILES})

# Link against required libraries
target_link_libraries(gui_test PRIVATE
    ${GTEST_LIB_FILES}
)

# Add compile definitions for debug port
target_compile_definitions(gui_test PRIVATE -DENABLE_DEBUG_PORT)

# Set C++ standard
target_compile_features(gui_test PUBLIC cxx_std_17)

# Add test to CTest
add_test(NAME gui_test COMMAND gui_test)

# Set test environment - GUI tests need a display
set_tests_properties(gui_test PROPERTIES
    ENVIRONMENT "DISPLAY=:0"
    TIMEOUT 30
)