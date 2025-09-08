#ifdef ENABLE_DEBUG_PORT

#include "debug_tcp_server.h"
#include <QMetaObject>
#include <QCoreApplication>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

DebugTcpServer::DebugTcpServer(int port, PythonREPLWidget* widget) 
    : running(false), port(port), server_socket(-1), replWidget(widget) {
}

DebugTcpServer::~DebugTcpServer() {
    stop();
}

bool DebugTcpServer::start() {
    if (running.load()) {
        return false;
    }
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create debug socket" << std::endl;
        return false;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set debug socket options" << std::endl;
        close(server_socket);
        return false;
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost only for security
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind debug socket to port " << port << std::endl;
        close(server_socket);
        return false;
    }
    
    if (listen(server_socket, 5) < 0) {
        std::cerr << "Failed to listen on debug socket" << std::endl;
        close(server_socket);
        return false;
    }
    
    running.store(true);
    server_thread = std::thread(&DebugTcpServer::serverLoop, this);
    
    std::cout << "Debug TCP server started on port " << port << " (localhost only)" << std::endl;
    return true;
}

void DebugTcpServer::stop() {
    if (running.load()) {
        running.store(false);
        
        if (server_socket >= 0) {
            close(server_socket);
            server_socket = -1;
        }
        
        if (server_thread.joinable()) {
            server_thread.join();
        }
        
        std::cout << "Debug TCP server stopped" << std::endl;
    }
}

bool DebugTcpServer::isRunning() const {
    return running.load();
}

void DebugTcpServer::setReplWidget(PythonREPLWidget* widget) {
    replWidget = widget;
}

void DebugTcpServer::serverLoop() {
    std::cout << "Debug TCP server loop started, waiting for connections..." << std::endl;
    while (running.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running.load()) {
                std::cerr << "Failed to accept debug client connection" << std::endl;
            }
            continue;
        }
        
        std::cout << "Debug client connected" << std::endl;
        handleClient(client_socket);
        close(client_socket);
        std::cout << "Debug client disconnected" << std::endl;
    }
}

void DebugTcpServer::handleClient(int client_socket) {
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    
    // Read command from client
    ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        std::cerr << "Failed to read debug command" << std::endl;
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string commandStr(buffer);
    
    try {
        // Parse JSON command
        nlohmann::json command = nlohmann::json::parse(commandStr);
        
        // Process command
        nlohmann::json response = processCommand(command);
        
        // Send JSON response
        std::string responseStr = response.dump();
        send(client_socket, responseStr.c_str(), responseStr.length(), 0);
        
    } catch (const std::exception& e) {
        // Send error response
        nlohmann::json errorResponse = {
            {"status", "error"},
            {"message", std::string("Invalid command: ") + e.what()}
        };
        std::string responseStr = errorResponse.dump();
        send(client_socket, responseStr.c_str(), responseStr.length(), 0);
    }
}

// processCommand implementation moved to main.cpp to avoid circular dependency

#endif // ENABLE_DEBUG_PORT