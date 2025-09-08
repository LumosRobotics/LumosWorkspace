cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CPP_SOURCE_FILES main.cpp)

add_executable(repl ${CPP_SOURCE_FILES})

# Link Python library and required system libraries
find_library(INTL_LIB intl PATHS /opt/homebrew/lib /usr/local/lib)
if(INTL_LIB)
    target_link_libraries(repl PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        tcp_server
        ${INTL_LIB}
        dl
        util
        m
    )
else()
    # Fallback: try without intl library (some systems have it built-in)
    target_link_libraries(repl PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        tcp_server
        dl
        util
        m
    )
endif()
