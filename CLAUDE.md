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
- **Qt6** (optional): For GUI applications - detected automatically by CMake
- **nlohmann/json**: For TCP protocol serialization - header-only library in `third_party/`
- **gettext/libintl**: Required for Python's locale module support

### Building the Project

```bash
# Create build directory and configure
mkdir -p build && cd build
cmake ..

# Build the project (use parallel jobs for faster builds)
make -j6

# Available applications:
./src/applications/simple/simple           # Basic Python embedding demo
./src/applications/repl/repl               # Command-line Python REPL
./src/applications/repl_gui/repl_gui       # Qt6 GUI REPL with TCP (requires Qt6)
./src/applications/repl_gui/repl_gui_modular # Modular version with separate components
./src/applications/simple_transmitter/simple_transmitter # Data transmission utility
```

### Running Tests

```bash
# From build directory
ctest
# Or run tests with verbose output
ctest -V

# Run individual module tests
./src/modules/tcp_server/test/tcp_server_test
./src/modules/tcp_client/test/tcp_client_test  
./src/modules/settings_handler/test/settings_handler_test
```

## Architecture

The project is organized into multiple applications and reusable modules:

### Applications
- `src/applications/simple/` - Basic Python embedding demonstration
- `src/applications/repl/` - Command-line Python REPL
- `src/applications/repl_gui/` - Qt6-based GUI Python REPL with TCP data injection
- `src/applications/simple_transmitter/` - Data transmission utilities
- `src/applications/gui_test/` - GUI component testing

### Core Modules
- `src/modules/python_engine.*` - Encapsulated Python interpreter management
- `src/modules/tcp_server/` - TCP server for real-time data injection
- `src/modules/tcp_client/` - TCP client utilities  
- `src/modules/settings_handler/` - Cross-platform settings persistence
- `src/modules/main_window.*` - Main Qt window management
- `src/modules/ui_theme_manager.*` - UI theming system
- `src/modules/variables_panel.*` - Python variables display

### Third-Party Dependencies
- `third_party/cpython/` - Full CPython 3.13 source and build artifacts
- `third_party/googletest/` - Google Test framework
- `third_party/nlohmann/` - JSON library for TCP protocol
- `third_party/ApplicationDeployment/` - Deployment utilities

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
4. **GIL Management**: Thread-safe access to Python interpreter through `PyGILState_Ensure`/`PyGILState_Release`
5. **Memory Management**: Proper reference counting with `Py_INCREF`/`Py_DECREF` for all Python objects

## Development Workflow

1. Make changes to source files in `src/`
2. Build from the `build/` directory using `make -j6`
3. Run tests using `ctest`
4. Execute applications from the build directory
5. For Python integration issues, ensure CPython is properly built in `third_party/cpython/`

## TCP Data Injection Architecture

The project supports real-time data injection into the Python REPL via TCP:

### Design Overview

1. **TCP Server Thread**: A separate C++ thread runs a TCP server listening on a configurable port
2. **Message Protocol**: Each TCP message contains:
   - Header: JSON metadata describing data type and target variable name
   - Payload: Serialized data (JSON, binary, or custom format)
3. **Python Integration**: Received data is injected directly into Python's global namespace using the Python C API
4. **Thread Safety**: Proper GIL (Global Interpreter Lock) handling for thread-safe Python object creation

### Implementation Flow

```
TCP Client → [Header + Data] → C++ TCP Thread → Python C API → Python Global Namespace → Available in REPL
```

Example: Client sends `{"type": "int_list", "name": "received_data"}` + `[1,2,3,4,5]` → Python REPL user can immediately access `received_data` variable.

### Key Considerations

- **GIL Management**: TCP thread must acquire/release GIL when injecting Python objects
- **Serialization**: Support multiple formats (JSON for simple data, pickle for Python objects, custom binary for performance)
- **Error Handling**: Graceful handling of malformed data or Python injection failures
- **Memory Management**: Proper Python reference counting for injected objects

## GUI Application Features

The Qt6 GUI application (`repl_gui`) provides advanced features:

### Layout Modes
- **Bottom Input Mode**: Traditional REPL with input box at bottom
- **Inline Input Mode**: Input appears directly in output area at cursor position
- Switch modes via `repl_gui → Settings` menu

### Debug API
- Optional debug TCP server on port 8081 (enabled with `ENABLE_DEBUG_PORT`)
- JSON-based command protocol for automated testing
- Available commands: `execute`, `get_output`, `get_variables`, `clear_output`, `get_input`, `set_input`, `ping`

### Settings Persistence  
- Cross-platform settings storage via `SettingsHandler` module
- Persists window geometry, UI preferences, layout mode, and color themes
- Settings locations: macOS (`~/Library/Application Support/`), Linux (`~/.config/`), Windows (`AppData`)

## Important Notes

- The Python interpreter runs in the same process as the C++ application
- Python path is set relative to the executable location
- All Python objects require proper reference counting and cleanup
- The integration supports full bidirectional data exchange between C++ and Python
- TCP data injection requires careful thread synchronization due to Python's GIL
- Qt6 is optional - project builds successfully without it, skipping GUI applications
- Debug features are conditional compilation controlled by `ENABLE_DEBUG_PORT` CMake option