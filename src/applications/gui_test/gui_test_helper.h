#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class GuiTestHelper {
private:
    int gui_pid;
    std::string gui_executable_path;
    bool gui_started;
    
    bool waitForDebugPort(int timeout_seconds = 10);
    nlohmann::json sendDebugCommand(const nlohmann::json& command);
    
public:
    GuiTestHelper(const std::string& executable_path);
    ~GuiTestHelper();
    
    // GUI application management
    bool startGui();
    void stopGui();
    bool isGuiRunning() const;
    
    // Debug API commands
    nlohmann::json ping();
    nlohmann::json executeCode(const std::string& code);
    nlohmann::json getOutput();
    nlohmann::json getVariables();
    nlohmann::json clearOutput();
    nlohmann::json getInput();
    nlohmann::json setInput(const std::string& text);
    
    // Convenience methods
    bool waitForVariableCount(int expected_count, int timeout_seconds = 5);
    bool outputContains(const std::string& text);
    std::vector<std::string> getVariableList();
};