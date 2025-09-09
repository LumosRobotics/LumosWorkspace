#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <utility>
#include <cstdlib>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <Python.h>
// #include <nlohmann/json.hpp>

class TerminalUI {
private:
    int terminal_width;
    int terminal_height;
    int left_width;
    int right_width;
    struct termios original_termios;
    
public:
    TerminalUI() {
        getTerminalSize();
        left_width = terminal_width * 2 / 3;  // 2/3 for REPL
        right_width = terminal_width - left_width - 1;  // 1/3 for variables (minus separator)
        setupRawMode();
    }
    
    ~TerminalUI() {
        restoreTerminalMode();
    }
    
    void getTerminalSize() {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        terminal_width = w.ws_col;
        terminal_height = w.ws_row;
    }
    
    void setupRawMode() {
        tcgetattr(STDIN_FILENO, &original_termios);
    }
    
    void restoreTerminalMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    }
    
    void clearScreen() {
        std::cout << "\033[2J\033[H" << std::flush;
    }
    
    void drawBorder() {
        // Draw vertical separator at left_width position
        for (int row = 1; row <= terminal_height; row++) {
            moveCursor(row, left_width + 1);
            std::cout << "│";
        }
        
        // Draw header for variables panel
        moveCursor(1, left_width + 3);
        std::cout << "Variables";
        moveCursor(2, left_width + 2);
        for (int i = 0; i < right_width - 1; i++) {
            std::cout << "─";
        }
    }
    
    void moveCursor(int row, int col) {
        std::cout << "\033[" << row << ";" << col << "H" << std::flush;
    }
    
    void updateVariablesPanel(const std::vector<std::string>& variables) {
        // Clear variables panel area
        for (int row = 3; row <= terminal_height; row++) {
            moveCursor(row, left_width + 2);
            for (int col = 0; col < right_width; col++) {
                std::cout << " ";
            }
        }
        
        // Display variables
        int row = 3;
        for (const auto& var : variables) {
            if (row > terminal_height - 1) break;
            moveCursor(row, left_width + 2);
            
            // Truncate if too long
            std::string display_var = var;
            if (display_var.length() > static_cast<size_t>(right_width - 1)) {
                display_var = display_var.substr(0, right_width - 4) + "...";
            }
            
            std::cout << display_var;
            row++;
        }
        
        std::cout << std::flush;
    }
    
    void showPrompt(int current_row) {
        moveCursor(current_row, 1);
        std::cout << ">>> " << std::flush;
    }
    
    int getLeftWidth() const { return left_width; }
    int getTerminalHeight() const { return terminal_height; }
};

std::vector<std::string> getUserVariables() {
    std::vector<std::string> variables;
    
    // Get main module dictionary
    PyObject* main_module = PyImport_AddModule("__main__");
    PyObject* main_dict = PyModule_GetDict(main_module);
    
    if (!main_dict) return variables;
    
    PyObject* keys = PyDict_Keys(main_dict);
    if (!keys) return variables;
    
    Py_ssize_t size = PyList_Size(keys);
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject* key = PyList_GetItem(keys, i);
        if (!key) continue;
        
        const char* key_str = PyUnicode_AsUTF8(key);
        if (!key_str) continue;
        
        // Skip built-in variables that start with __
        std::string key_name(key_str);
        if (key_name.substr(0, 2) == "__") continue;
        
        // Get the value and its type
        PyObject* value = PyDict_GetItem(main_dict, key);
        if (value) {
            PyObject* type_obj = PyObject_Type(value);
            PyObject* type_name = PyObject_GetAttrString(type_obj, "__name__");
            PyObject* repr_obj = PyObject_Repr(value);
            
            if (type_name && repr_obj) {
                const char* type_str = PyUnicode_AsUTF8(type_name);
                const char* repr_str = PyUnicode_AsUTF8(repr_obj);
                
                if (type_str && repr_str) {
                    std::string var_info = key_name + ": " + type_str;
                    
                    // Add value if it's not too long
                    std::string value_str(repr_str);
                    if (value_str.length() < 30) {
                        var_info += " = " + value_str;
                    }
                    
                    variables.push_back(var_info);
                }
            }
            
            Py_XDECREF(type_obj);
            Py_XDECREF(type_name);
            Py_XDECREF(repr_obj);
        }
    }
    
    Py_DECREF(keys);
    return variables;
}

std::string evaluatePythonExpression(const std::string& expression) {
    // Get main module and its dictionary for persistent state
    PyObject* main_module = PyImport_AddModule("__main__");
    PyObject* main_dict = PyModule_GetDict(main_module);
    
    // Try to evaluate as expression first
    PyObject* result = PyRun_String(expression.c_str(), Py_eval_input, main_dict, main_dict);
    
    if (result) {
        // Expression evaluation succeeded
        if (result != Py_None) {
            PyObject* repr = PyObject_Repr(result);
            if (repr) {
                const char* result_str = PyUnicode_AsUTF8(repr);
                std::string output = result_str ? result_str : "";
                Py_DECREF(repr);
                Py_DECREF(result);
                return output;
            }
        }
        Py_DECREF(result);
        return "";
    } else {
        // Clear error and try as statement
        PyErr_Clear();
        
        // For statements, just execute them
        result = PyRun_String(expression.c_str(), Py_single_input, main_dict, main_dict);
        
        if (result) {
            Py_DECREF(result);
            return "";
        } else {
            // Handle error
            if (PyErr_Occurred()) {
                PyObject* exc_type, *exc_value, *exc_traceback;
                PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
                
                std::string error_msg = "Error: ";
                if (exc_value) {
                    PyObject* str_exc = PyObject_Str(exc_value);
                    if (str_exc) {
                        const char* msg = PyUnicode_AsUTF8(str_exc);
                        if (msg) {
                            error_msg += msg;
                        }
                        Py_DECREF(str_exc);
                    }
                }
                
                Py_XDECREF(exc_type);
                Py_XDECREF(exc_value);
                Py_XDECREF(exc_traceback);
                
                return error_msg;
            }
            
            return "Error: Unknown execution error";
        }
    }
}

int main()
{
    // Set Python path environment variable to include site-packages and modules
    setenv("PYTHONPATH", "../third_party/cpython/Lib:../third_party/cpython/Lib/site-packages:../third_party/cpython/Modules", 1);
    setenv("PYTHONHOME", "../third_party/cpython", 1);
    
    // Initialize Python interpreter
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        std::cerr << "Failed to initialize Python interpreter" << std::endl;
        return 1;
    }
    
    // Create terminal UI
    TerminalUI ui;
    ui.clearScreen();
    
    // Draw initial interface
    ui.moveCursor(1, 1);
    std::cout << "Python REPL with Variable Monitor";
    ui.moveCursor(2, 1);
    std::cout << "Type 'exit()' or 'quit()' to exit, Ctrl+C to interrupt";
    ui.moveCursor(3, 1);
    for (int i = 0; i < ui.getLeftWidth(); i++) {
        std::cout << "─";
    }
    
    ui.drawBorder();
    
    int current_row = 5;
    std::string input;
    
    while (true) {
        // Update variables panel
        auto variables = getUserVariables();
        ui.updateVariablesPanel(variables);
        
        // Show prompt on left side
        ui.showPrompt(current_row);
        
        if (!std::getline(std::cin, input)) {
            // EOF reached (Ctrl+D)
            std::cout << std::endl;
            break;
        }
        
        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t\r\n"));
        input.erase(input.find_last_not_of(" \t\r\n") + 1);
        
        // Check for exit commands
        if (input == "exit()" || input == "quit()" || input == "exit" || input == "quit") {
            break;
        }
        
        // Skip empty lines
        if (input.empty()) {
            current_row++;
            if (current_row > ui.getTerminalHeight() - 2) {
                ui.clearScreen();
                ui.drawBorder();
                current_row = 5;
            }
            continue;
        }
        
        // Move to next line for output
        current_row++;
        ui.moveCursor(current_row, 1);
        
        // Evaluate Python expression/statement
        std::string result = evaluatePythonExpression(input);
        
        // Print result if not empty
        if (!result.empty()) {
            // Ensure output stays in left panel
            std::istringstream result_stream(result);
            std::string line;
            while (std::getline(result_stream, line)) {
                ui.moveCursor(current_row, 1);
                
                // Truncate line if too long for left panel
                if (line.length() > static_cast<size_t>(ui.getLeftWidth() - 1)) {
                    line = line.substr(0, ui.getLeftWidth() - 4) + "...";
                }
                
                std::cout << line;
                current_row++;
                
                if (current_row > ui.getTerminalHeight() - 2) {
                    ui.clearScreen();
                    ui.drawBorder();
                    current_row = 5;
                    break;
                }
            }
        }
        
        // Move to next line for next prompt
        current_row++;
        if (current_row > ui.getTerminalHeight() - 2) {
            ui.clearScreen();
            ui.drawBorder();
            current_row = 5;
        }
    }
    
    // Restore terminal and exit
    ui.clearScreen();
    std::cout << "Goodbye!" << std::endl;
    
    // Clean up and finalize Python interpreter
    Py_Finalize();
    
    return 0;
}