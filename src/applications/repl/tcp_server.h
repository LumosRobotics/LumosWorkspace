#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <string>

class TCPServer {
private:
    std::thread server_thread;
    std::atomic<bool> running;
    int port;
    int server_socket;
    
    void serverLoop();
    void handleClient(int client_socket);
    
public:
    TCPServer(int port = 8080);
    ~TCPServer();
    
    bool start();
    void stop();
    bool isRunning() const;
    
    // Callback for when data is received
    // Parameters: header (JSON string), payload (raw data)
    std::function<void(const std::string&, const std::string&)> onDataReceived;
};