cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, skipping repl_gui")
    return()
endif()

set(CPP_SOURCE_FILES 
    main.cpp 
)

# Add debug server files if enabled
if(ENABLE_DEBUG_PORT)
    list(APPEND CPP_SOURCE_FILES debug_tcp_server.cpp)
    add_definitions(-DENABLE_DEBUG_PORT)
endif()

add_executable(repl_gui ${CPP_SOURCE_FILES})

# Qt6 setup
set_target_properties(repl_gui PROPERTIES
    AUTOMOC ON
)

target_link_libraries(repl_gui PRIVATE
    Qt6::Core
    Qt6::Widgets
    tcp_server
    settings_handler
)

# Link Python library and required system libraries
find_library(INTL_LIB intl PATHS /opt/homebrew/lib /usr/local/lib)
if(INTL_LIB)
    target_link_libraries(repl_gui PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        ${INTL_LIB}
        dl
        util
        m
    )
else()
    # Fallback: try without intl library (some systems have it built-in)
    target_link_libraries(repl_gui PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        dl
        util
        m
    )
endif()