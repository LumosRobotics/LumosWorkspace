#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <utility>
#include <cstdlib>
#include <Python.h>
// #include <nlohmann/json.hpp>

int main()
{
    // Set Python path environment variable
    setenv("PYTHONPATH", "../third_party/cpython/Lib", 1);
    setenv("PYTHONHOME", "../third_party/cpython", 1);
    
    // Initialize Python interpreter
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        std::cerr << "Failed to initialize Python interpreter" << std::endl;
        return 1;
    }
    
    std::cout << "Python interpreter initialized successfully!" << std::endl;
    std::cout << "Python version: " << Py_GetVersion() << std::endl;
    
    // Execute a simple Python script
    const char* python_script = 
        "import sys\n"
        "print(f'Python {sys.version}')\n"
        "print('Hello from embedded Python!')\n"
        "result = 2 + 3\n"
        "print(f'2 + 3 = {result}')\n";
    
    int result = PyRun_SimpleString(python_script);
    if (result != 0) {
        std::cerr << "Error executing Python script" << std::endl;
    }
    
    // Execute Python code that interacts with C++ data
    PyObject* pModule = PyImport_AddModule("__main__");
    PyObject* pDict = PyModule_GetDict(pModule);
    
    // Pass a value from C++ to Python
    int cpp_value = 42;
    PyObject* py_value = PyLong_FromLong(cpp_value);
    PyDict_SetItemString(pDict, "cpp_value", py_value);
    Py_DECREF(py_value);
    
    // Execute Python code using the C++ value
    PyRun_SimpleString("print(f'Value from C++: {cpp_value}')");
    PyRun_SimpleString("squared = cpp_value * cpp_value");
    PyRun_SimpleString("print(f'{cpp_value} squared is {squared}')");
    
    // Get a result back from Python
    PyObject* py_squared = PyDict_GetItemString(pDict, "squared");
    if (py_squared && PyLong_Check(py_squared)) {
        long squared_value = PyLong_AsLong(py_squared);
        std::cout << "Retrieved squared value from Python: " << squared_value << std::endl;
    }
    
    // Clean up and finalize Python interpreter
    Py_Finalize();
    
    return 0;
}