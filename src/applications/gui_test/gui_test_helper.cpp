#include "gui_test_helper.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

GuiTestHelper::GuiTestHelper(const std::string& executable_path) 
    : gui_pid(-1), gui_executable_path(executable_path), gui_started(false) {
}

GuiTestHelper::~GuiTestHelper() {
    stopGui();
}

bool GuiTestHelper::startGui() {
    if (gui_started) {
        return true;
    }
    
    std::cout << "Starting GUI application: " << gui_executable_path << std::endl;
    
    gui_pid = fork();
    if (gui_pid == 0) {
        // Child process - exec the GUI application
        // Redirect stdout/stderr to avoid cluttering test output
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        execl(gui_executable_path.c_str(), "repl_gui", nullptr);
        // If we get here, exec failed
        std::cerr << "Failed to exec GUI application" << std::endl;
        exit(1);
    } else if (gui_pid > 0) {
        // Parent process - wait for GUI to start
        gui_started = true;
        
        if (!waitForDebugPort()) {
            std::cerr << "GUI started but debug port not available" << std::endl;
            stopGui();
            return false;
        }
        
        std::cout << "GUI application started successfully (PID: " << gui_pid << ")" << std::endl;
        return true;
    } else {
        // Fork failed
        std::cerr << "Failed to fork GUI process" << std::endl;
        return false;
    }
}

void GuiTestHelper::stopGui() {
    if (gui_started && gui_pid > 0) {
        std::cout << "Stopping GUI application (PID: " << gui_pid << ")" << std::endl;
        
        // Send SIGTERM first
        kill(gui_pid, SIGTERM);
        
        // Wait up to 5 seconds for graceful shutdown
        int status;
        for (int i = 0; i < 50; ++i) {
            if (waitpid(gui_pid, &status, WNOHANG) == gui_pid) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // If still running, force kill
        if (waitpid(gui_pid, &status, WNOHANG) == 0) {
            std::cout << "Force killing GUI application" << std::endl;
            kill(gui_pid, SIGKILL);
            waitpid(gui_pid, &status, 0);
        }
        
        gui_pid = -1;
        gui_started = false;
        std::cout << "GUI application stopped" << std::endl;
    }
}

bool GuiTestHelper::isGuiRunning() const {
    if (!gui_started || gui_pid <= 0) {
        return false;
    }
    
    // Check if process is still alive
    int status;
    int result = waitpid(gui_pid, &status, WNOHANG);
    return result == 0; // 0 means process is still running
}

bool GuiTestHelper::waitForDebugPort(int timeout_seconds) {
    std::cout << "Waiting for debug port to become available..." << std::endl;
    
    for (int i = 0; i < timeout_seconds * 10; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }
        
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_addr.sin_port = htons(8081);
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            close(sock);
            std::cout << "Debug port is available" << std::endl;
            return true;
        }
        
        close(sock);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Debug port not available after " << timeout_seconds << " seconds" << std::endl;
    return false;
}

nlohmann::json GuiTestHelper::sendDebugCommand(const nlohmann::json& command) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return {{"status", "error"}, {"message", "Failed to create socket"}};
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8081);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return {{"status", "error"}, {"message", "Failed to connect to debug port"}};
    }
    
    // Send command
    std::string command_str = command.dump();
    if (send(sock, command_str.c_str(), command_str.length(), 0) < 0) {
        close(sock);
        return {{"status", "error"}, {"message", "Failed to send command"}};
    }
    
    // Receive response
    char buffer[8192];
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    close(sock);
    
    if (bytes_received <= 0) {
        return {{"status", "error"}, {"message", "Failed to receive response"}};
    }
    
    buffer[bytes_received] = '\0';
    
    try {
        return nlohmann::json::parse(buffer);
    } catch (const std::exception& e) {
        return {{"status", "error"}, {"message", std::string("Failed to parse response: ") + e.what()}};
    }
}

// Debug API command methods
nlohmann::json GuiTestHelper::ping() {
    return sendDebugCommand({{"command", "ping"}});
}

nlohmann::json GuiTestHelper::executeCode(const std::string& code) {
    return sendDebugCommand({{"command", "execute"}, {"code", code}});
}

nlohmann::json GuiTestHelper::getOutput() {
    return sendDebugCommand({{"command", "get_output"}});
}

nlohmann::json GuiTestHelper::getVariables() {
    return sendDebugCommand({{"command", "get_variables"}});
}

nlohmann::json GuiTestHelper::clearOutput() {
    return sendDebugCommand({{"command", "clear_output"}});
}

nlohmann::json GuiTestHelper::getInput() {
    return sendDebugCommand({{"command", "get_input"}});
}

nlohmann::json GuiTestHelper::setInput(const std::string& text) {
    return sendDebugCommand({{"command", "set_input"}, {"text", text}});
}

// Convenience methods
bool GuiTestHelper::waitForVariableCount(int expected_count, int timeout_seconds) {
    for (int i = 0; i < timeout_seconds * 10; ++i) {
        auto response = getVariables();
        if (response["status"] == "success") {
            auto variables = response["variables"];
            if (variables.size() == expected_count) {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

bool GuiTestHelper::outputContains(const std::string& text) {
    auto response = getOutput();
    if (response["status"] == "success") {
        std::string output = response["output"];
        return output.find(text) != std::string::npos;
    }
    return false;
}

std::vector<std::string> GuiTestHelper::getVariableList() {
    auto response = getVariables();
    std::vector<std::string> variables;
    
    if (response["status"] == "success") {
        for (const auto& var : response["variables"]) {
            variables.push_back(var.get<std::string>());
        }
    }
    
    return variables;
}