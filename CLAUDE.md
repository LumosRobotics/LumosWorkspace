# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LumosWorkspace is a C++ project that integrates an embedded Python interpreter within the same process. The project demonstrates how to embed CPython in a C++ application, allowing for bidirectional communication between C++ and Python code.

## Build System

This project uses CMake with the following configuration:
- C++17 standard
- Debug build type by default
- Includes Google Test for testing framework
- **Python Integration**: Embeds CPython 3.13 for in-process Python execution
- Uses static linking with libpython3.13.a

### Dependencies

- **CPython 3.13**: Located in `third_party/cpython/` - provides the embedded Python interpreter
- **Google Test**: Located in `third_party/googletest/` - testing framework
- **gettext/libintl**: Required for Python's locale module support

### Building the Project

```bash
# Create build directory and configure
mkdir -p build && cd build
cmake ..

# Build the project (use parallel jobs for faster builds)
make -j6

# Run the simple application with Python integration
./src/applications/simple/simple
```

### Running Tests

```bash
# From build directory
ctest
# Or run tests with verbose output
ctest -V
```

## Architecture

The project follows a standard C++ project structure with Python integration:

- `src/applications/simple/` - Contains the main application demonstrating Python embedding
- `third_party/cpython/` - Full CPython 3.13 source and build artifacts
- `third_party/googletest/` - Google Test framework
- `CMakeLists.txt` - Root CMake configuration with Python support
- `build/` - Build artifacts (generated)

## Python Integration Details

The main application (`src/applications/simple/main.cpp`) demonstrates:
- Python interpreter initialization and cleanup
- Execution of Python scripts from C++
- Passing data from C++ to Python environment
- Retrieving computed results from Python back to C++
- Proper memory management with Python objects

### Python Environment Setup

The application automatically configures:
- `PYTHONPATH` pointing to the embedded Python standard library
- `PYTHONHOME` pointing to the CPython installation directory

### Key Integration Points

1. **Headers**: `third_party/cpython/Include/` and `third_party/cpython/` for `pyconfig.h`
2. **Libraries**: Links against `libpython3.13.a` and required system libraries (intl, dl, util, m)
3. **Runtime**: Sets environment variables to locate Python standard library

## Development Workflow

1. Make changes to source files in `src/`
2. Build from the `build/` directory using `make -j6`
3. Run tests using `ctest`
4. Execute applications from the build directory
5. For Python integration issues, ensure CPython is properly built in `third_party/cpython/`

## Important Notes

- The Python interpreter runs in the same process as the C++ application
- Python path is set relative to the executable location
- All Python objects require proper reference counting and cleanup
- The integration supports full bidirectional data exchange between C++ and Python