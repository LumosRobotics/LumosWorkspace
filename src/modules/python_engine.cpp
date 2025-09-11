#include "python_engine.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

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
    std::string executablePath = getExecutablePath();
    std::string baseDir;
    std::string pythonLib;
    
    // Check if we're running from an app bundle
    if (executablePath.find(".app/Contents/MacOS/") != std::string::npos) {
        // Extract path to the .app bundle directory
        size_t appPos = executablePath.find(".app/Contents/MacOS/");
        std::string appBundlePath = executablePath.substr(0, appPos + 4); // Include .app
        
        // First try to use bundled Python library within the app
        std::string bundledPythonLib = appBundlePath + "/Contents/Resources/python_lib";
        if (std::filesystem::exists(bundledPythonLib)) {
            // Use bundled Python library
            pythonLib = bundledPythonLib;
            baseDir = appBundlePath + "/Contents/Resources";
            
            // Set up paths using bundled library
            std::string pythonPath = pythonLib;
            setenv("PYTHONPATH", pythonPath.c_str(), 1);
            
            // For PYTHONHOME, we need to simulate the structure Python expects
            // Create a temporary structure or use the bundle path
            setenv("PYTHONHOME", baseDir.c_str(), 1);
            
            return;
        }
        
        // Fallback: search for original build directory structure
        std::vector<std::string> searchPaths = {
            appBundlePath + "/../../../third_party/cpython",  // From build/src/applications/repl_gui/app.app
            appBundlePath + "/../../third_party/cpython",     // Alternative location
            appBundlePath + "/../third_party/cpython",        // Another alternative
        };
        
        bool foundPython = false;
        for (const auto& searchPath : searchPaths) {
            std::filesystem::path pythonLibPath = std::filesystem::path(searchPath) / "Lib";
            if (std::filesystem::exists(pythonLibPath)) {
                baseDir = searchPath;
                foundPython = true;
                break;
            }
        }
        
        if (!foundPython) {
            // Final fallback
            baseDir = "../third_party/cpython";
        }
    } else {
        // Running from build directory, use relative paths
        baseDir = "../third_party/cpython";
    }
    
    // Construct Python paths for non-bundled case
    pythonLib = baseDir + "/Lib";
    std::string pythonSitePackages = baseDir + "/Lib/site-packages";  
    std::string pythonModules = baseDir + "/Modules";
    std::string pythonPath = pythonLib + ":" + pythonSitePackages + ":" + pythonModules;
    
    setenv("PYTHONPATH", pythonPath.c_str(), 1);
    setenv("PYTHONHOME", baseDir.c_str(), 1);
}

std::string PythonEngine::getExecutablePath() {
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::string(path);
    }
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::string(path);
    }
#endif
    return "";
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