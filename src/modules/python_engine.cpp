#include "python_engine.h"
#include <iostream>
#include <sstream>

PythonEngine::PythonEngine() : initialized(false) {}

PythonEngine::~PythonEngine() {
    if (initialized) {
        finalize();
    }
}

bool PythonEngine::initialize() {
    if (initialized) {
        return true;
    }
    
    setupPythonPath();
    
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        std::cerr << "Failed to initialize Python interpreter" << std::endl;
        return false;
    }
    
    initialized = true;
    return true;
}

void PythonEngine::finalize() {
    if (initialized) {
        Py_Finalize();
        initialized = false;
    }
}

void PythonEngine::setupPythonPath() {
    // Set Python path environment variable to include site-packages and modules
    setenv("PYTHONPATH", "../third_party/cpython/Lib:../third_party/cpython/Lib/site-packages:../third_party/cpython/Modules", 1);
    setenv("PYTHONHOME", "../third_party/cpython", 1);
}

std::string PythonEngine::evaluateExpression(const std::string& expression) {
    if (!initialized) {
        return "Error: Python engine not initialized";
    }
    
    // Get main module and its dictionary for persistent state
    PyObject* main_module = PyImport_AddModule("__main__");
    PyObject* main_dict = PyModule_GetDict(main_module);
    
    // Redirect stdout to capture print output
    PyObject* io_module = PyImport_ImportModule("io");
    PyObject* string_io = PyObject_CallMethod(io_module, "StringIO", NULL);
    PyObject* sys_module = PyImport_ImportModule("sys");
    PyObject* old_stdout = PyObject_GetAttrString(sys_module, "stdout");
    PyObject_SetAttrString(sys_module, "stdout", string_io);
    
    std::string output = "";
    
    // Try to evaluate as expression first
    PyObject* result = PyRun_String(expression.c_str(), Py_eval_input, main_dict, main_dict);
    
    if (result) {
        // Expression evaluation succeeded
        if (result != Py_None) {
            PyObject* repr = PyObject_Repr(result);
            if (repr) {
                const char* result_str = PyUnicode_AsUTF8(repr);
                output = result_str ? result_str : "";
                Py_DECREF(repr);
            }
        }
        Py_DECREF(result);
    } else {
        // Clear error and try as statement or multi-line code
        PyErr_Clear();
        
        // For multi-line code blocks, use Py_file_input
        result = PyRun_String(expression.c_str(), Py_file_input, main_dict, main_dict);
        
        if (!result) {
            // If file input fails, try single input for simple statements
            PyErr_Clear();
            result = PyRun_String(expression.c_str(), Py_single_input, main_dict, main_dict);
        }
        
        if (result) {
            Py_DECREF(result);
        } else {
            // Restore stdout before returning error
            PyObject_SetAttrString(sys_module, "stdout", old_stdout);
            Py_DECREF(old_stdout);
            Py_DECREF(string_io);
            Py_DECREF(sys_module);
            Py_DECREF(io_module);
            return formatPythonError();
        }
    }
    
    // Get captured output from StringIO
    PyObject* captured_output = PyObject_CallMethod(string_io, "getvalue", NULL);
    if (captured_output) {
        const char* captured_str = PyUnicode_AsUTF8(captured_output);
        if (captured_str && strlen(captured_str) > 0) {
            if (!output.empty()) {
                output += "\n" + std::string(captured_str);
            } else {
                output = captured_str;
            }
            // Remove trailing newline if present
            if (!output.empty() && output.back() == '\n') {
                output.pop_back();
            }
        }
        Py_DECREF(captured_output);
    }
    
    // Restore stdout
    PyObject_SetAttrString(sys_module, "stdout", old_stdout);
    Py_DECREF(old_stdout);
    Py_DECREF(string_io);
    Py_DECREF(sys_module);
    Py_DECREF(io_module);
    
    return output;
}

std::vector<PythonVariable> PythonEngine::getUserVariables() {
    std::vector<PythonVariable> variables;
    
    if (!initialized) {
        return variables;
    }
    
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
            PythonVariable var;
            var.name = key_name;
            
            PyObject* type_obj = PyObject_Type(value);
            PyObject* type_name = PyObject_GetAttrString(type_obj, "__name__");
            PyObject* repr_obj = PyObject_Repr(value);
            
            if (type_name && repr_obj) {
                const char* type_str = PyUnicode_AsUTF8(type_name);
                const char* repr_str = PyUnicode_AsUTF8(repr_obj);
                
                if (type_str && repr_str) {
                    var.type = type_str;
                    var.value = repr_str;
                    
                    var.displayString = var.name + ": " + var.type;
                    
                    // Add value if it's not too long
                    if (var.value.length() < 100) {
                        var.displayString += " = " + var.value;
                    }
                    
                    variables.push_back(var);
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

void PythonEngine::acquireGIL() {
    if (initialized) {
        PyGILState_Ensure();
    }
}

void PythonEngine::releaseGIL() {
    if (initialized) {
        PyGILState_Release(PyGILState_Ensure());
    }
}

std::string PythonEngine::formatPythonError() {
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

std::string PythonEngine::truncateString(const std::string& str, size_t maxLength) {
    if (str.length() <= maxLength) {
        return str;
    }
    return str.substr(0, maxLength - 3) + "...";
}