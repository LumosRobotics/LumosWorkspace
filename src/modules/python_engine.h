#pragma once

#include <string>
#include <vector>

// Prevent Python/Qt slot keyword conflicts
#ifdef slots
#undef slots
#endif
#include <Python.h>
#define slots Q_SLOTS

struct PythonVariable {
    std::string name;
    std::string type;
    std::string value;
    std::string displayString;
};

class PythonEngine {
public:
    PythonEngine();
    ~PythonEngine();

    bool initialize();
    void finalize();
    
    std::string evaluateExpression(const std::string& expression);
    std::vector<PythonVariable> getUserVariables();
    
    bool isInitialized() const { return initialized; }
    
    // Thread-safe operations
    void acquireGIL();
    void releaseGIL();
    
private:
    bool initialized;
    void setupPythonPath();
    std::string formatPythonError();
    std::string truncateString(const std::string& str, size_t maxLength = 30);
};