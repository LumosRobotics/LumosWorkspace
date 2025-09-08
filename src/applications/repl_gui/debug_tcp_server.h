#pragma once

#ifdef ENABLE_DEBUG_PORT

#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <nlohmann/json.hpp>

class PythonREPLWidget;

class DebugTcpServer {
private:
    std::thread server_thread;
    std::atomic<bool> running;
    int port;
    int server_socket;
    PythonREPLWidget* replWidget;
    
    void serverLoop();
    void handleClient(int client_socket);
    
public:
    DebugTcpServer(int port = 8081, PythonREPLWidget* widget = nullptr);
    ~DebugTcpServer();
    
    bool start();
    void stop();
    bool isRunning() const;
    
    void setReplWidget(PythonREPLWidget* widget);
    
    // Process command function needs to be defined where PythonREPLWidget is complete
    nlohmann::json processCommand(const nlohmann::json& command);
};

#endif // ENABLE_DEBUG_PORT