cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, skipping repl_gui_modular")
    return()
endif()

# Module source files
set(MODULE_SOURCE_FILES 
    ../../modules/python_engine.cpp
    ../../modules/custom_title_bar.cpp
    ../../modules/settings_manager.cpp
    ../../modules/ui_theme_manager.cpp
    ../../modules/variables_panel.cpp
    ../../modules/repl_interface.cpp
    ../../modules/tcp_server.cpp
    ../../modules/debug_api.cpp
    ../../modules/layout_manager.cpp
    ../../modules/main_window.cpp
)

# Application source files
set(APP_SOURCE_FILES 
    main_modular.cpp 
)

# Create standalone binary for development
add_executable(LumosWorkspace ${APP_SOURCE_FILES} ${MODULE_SOURCE_FILES})

# Create modular executable (app bundle)
add_executable(repl_gui_modular ${APP_SOURCE_FILES} ${MODULE_SOURCE_FILES})

# Qt6 setup for both targets
set_target_properties(LumosWorkspace PROPERTIES
    AUTOMOC ON
)
set_target_properties(repl_gui_modular PROPERTIES
    AUTOMOC ON
)

# Include directories for both targets
target_include_directories(LumosWorkspace PRIVATE
    ${CMAKE_SOURCE_DIR}/src/modules
    ${CMAKE_SOURCE_DIR}/third_party/cpython/Include
    ${CMAKE_SOURCE_DIR}/third_party/cpython
)
target_include_directories(repl_gui_modular PRIVATE
    ${CMAKE_SOURCE_DIR}/src/modules
    ${CMAKE_SOURCE_DIR}/third_party/cpython/Include
    ${CMAKE_SOURCE_DIR}/third_party/cpython
)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Qt6 libraries for both targets
target_link_libraries(LumosWorkspace PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
target_link_libraries(repl_gui_modular PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Override build type for LumosWorkspace target - use Release optimizations
# even when project is built with Debug
target_compile_options(LumosWorkspace PRIVATE
    $<$<CONFIG:Debug>:-O2 -DNDEBUG>  # Use Release optimizations in Debug builds
)
target_compile_options(repl_gui_modular PRIVATE
    $<$<CONFIG:Debug>:-O2 -DNDEBUG>  # Use Release optimizations in Debug builds
)

# Python library and required system libraries for both targets
find_library(INTL_LIB intl PATHS /opt/homebrew/lib /usr/local/lib)
if(INTL_LIB)
    target_link_libraries(LumosWorkspace PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        ${INTL_LIB}
        dl
        util
        m
    )
    target_link_libraries(repl_gui_modular PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        ${INTL_LIB}
        dl
        util
        m
    )
else()
    # Fallback: try without intl library (some systems have it built-in)
    target_link_libraries(LumosWorkspace PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        dl
        util
        m
    )
    target_link_libraries(repl_gui_modular PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party/cpython/libpython3.13.a
        dl
        util
        m
    )
endif()

# Enable debug port if requested
if(ENABLE_DEBUG_PORT)
    add_definitions(-DENABLE_DEBUG_PORT)
endif()

# macOS Application Bundle Configuration
if(APPLE)
    # Set bundle properties
    set_target_properties(repl_gui_modular PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "LumosWorkspace"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_IDENTIFIER "com.lumosworkspace.app"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.lumosworkspace.app"
        MACOSX_BUNDLE_EXECUTABLE_NAME "LumosWorkspace"
        MACOSX_BUNDLE_ICON_FILE "AppIcon"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 LumosWorkspace. All rights reserved."
    )
    
    # Add application icon to bundle resources
    set(ICON_FILE "${CMAKE_SOURCE_DIR}/platform/macos/AppIcon.icns")
    if(EXISTS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES 
            MACOSX_PACKAGE_LOCATION Resources)
        target_sources(repl_gui_modular PRIVATE ${ICON_FILE})
        message(STATUS "Added application icon: ${ICON_FILE}")
    else()
        message(WARNING "Application icon not found: ${ICON_FILE}")
    endif()
    
    # Bundle Python library within the app for self-contained distribution
    set(PYTHON_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/cpython/Lib")
    if(EXISTS ${PYTHON_LIB_DIR})
        # Copy Python standard library to Resources/python_lib
        add_custom_command(TARGET repl_gui_modular POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory 
                "$<TARGET_BUNDLE_DIR:repl_gui_modular>/Contents/Resources/python_lib"
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${PYTHON_LIB_DIR}"
                "$<TARGET_BUNDLE_DIR:repl_gui_modular>/Contents/Resources/python_lib"
            COMMENT "Copying Python standard library to app bundle"
        )
        message(STATUS "Will bundle Python library from: ${PYTHON_LIB_DIR}")
    else()
        message(WARNING "Python library directory not found: ${PYTHON_LIB_DIR}")
    endif()
    
    # Set minimum macOS deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
    
    # Optional: Set code signing identity (uncomment if you have a developer certificate)
    # set_target_properties(repl_gui_modular PROPERTIES
    #     XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: Your Name"
    #     XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
    # )
    
    # Add install target for macOS Applications folder
    install(TARGETS repl_gui_modular
        BUNDLE DESTINATION "/Applications"
        COMPONENT Runtime
    )
    
    # Set the bundle name for installation
    set_target_properties(repl_gui_modular PROPERTIES
        OUTPUT_NAME "LumosWorkspace"
    )
    
    message(STATUS "Configured macOS application bundle for LumosWorkspace")
    message(STATUS "Use 'make install' to install to /Applications/LumosWorkspace.app")
endif()

# ApplicationDeployment integration for LumosWorkspace target
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ApplicationDeployment/deployment.cmake")
    include("${CMAKE_SOURCE_DIR}/third_party/ApplicationDeployment/deployment.cmake")
    
    # Apply deployment configuration to the LumosWorkspace target
    qt_deploy_application(LumosWorkspace
        MACOS_DEPLOYMENT_TARGET "10.15"
        MACOS_ARCHITECTURES "x86_64;arm64"  # Universal binary
    )
    
    message(STATUS "ApplicationDeployment configured for LumosWorkspace target")
    message(STATUS "Available deployment builds:")
    message(STATUS "  - macOS: ./third_party/ApplicationDeployment/build-parent.sh . LumosWorkspace macos monterey")
    message(STATUS "  - Linux: ./third_party/ApplicationDeployment/build-parent.sh . LumosWorkspace linux ubuntu 22.04")
    message(STATUS "  - Windows: ./third_party/ApplicationDeployment/build-parent.sh . LumosWorkspace windows")
else()
    message(STATUS "ApplicationDeployment not integrated - using standard build only")
    message(STATUS "To enable deployment: Ensure third_party/ApplicationDeployment/deployment.cmake exists")
endif()