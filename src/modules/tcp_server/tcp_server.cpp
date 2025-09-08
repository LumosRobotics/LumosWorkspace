#include "tcp_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

TCPServer::TCPServer(int port) : running(false), port(port), server_socket(-1) {
}

TCPServer::~TCPServer() {
    stop();
}

bool TCPServer::start() {
    if (running.load()) {
        return false;
    }
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket);
        return false;
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
        close(server_socket);
        return false;
    }
    
    if (listen(server_socket, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return false;
    }
    
    running.store(true);
    server_thread = std::thread(&TCPServer::serverLoop, this);
    
    std::cout << "TCP server started on port " << port << std::endl;
    return true;
}

void TCPServer::stop() {
    if (running.load()) {
        running.store(false);
        
        if (server_socket >= 0) {
            close(server_socket);
            server_socket = -1;
        }
        
        if (server_thread.joinable()) {
            server_thread.join();
        }
        
        std::cout << "TCP server stopped" << std::endl;
    }
}

bool TCPServer::isRunning() const {
    return running.load();
}

void TCPServer::serverLoop() {
    std::cout << "TCP server loop started, waiting for connections..." << std::endl;
    while (running.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running.load()) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        std::cout << "Client connected, handling request..." << std::endl;
        handleClient(client_socket);
        close(client_socket);
        std::cout << "Client disconnected" << std::endl;
    }
}

void TCPServer::handleClient(int client_socket) {
    std::cout << "Reading header size..." << std::endl;
    
    // Read header size first (4 bytes)
    uint32_t header_size;
    ssize_t bytes_read = recv(client_socket, &header_size, sizeof(header_size), MSG_WAITALL);
    if (bytes_read != sizeof(header_size)) {
        std::cerr << "Failed to read header size, got " << bytes_read << " bytes" << std::endl;
        return;
    }
    
    // Convert from network byte order
    header_size = ntohl(header_size);
    
    if (header_size > 1024) {
        std::cerr << "Header too large: " << header_size << std::endl;
        return;
    }
    
    // Read header
    std::string header(header_size, '\0');
    bytes_read = recv(client_socket, &header[0], header_size, MSG_WAITALL);
    if (bytes_read != static_cast<ssize_t>(header_size)) {
        std::cerr << "Failed to read header" << std::endl;
        return;
    }
    
    // Read payload size (4 bytes)
    uint32_t payload_size;
    bytes_read = recv(client_socket, &payload_size, sizeof(payload_size), MSG_WAITALL);
    if (bytes_read != sizeof(payload_size)) {
        std::cerr << "Failed to read payload size" << std::endl;
        return;
    }
    
    // Convert from network byte order
    payload_size = ntohl(payload_size);
    
    if (payload_size > 1024 * 1024) {  // 1MB limit
        std::cerr << "Payload too large: " << payload_size << std::endl;
        return;
    }
    
    // Read payload
    std::string payload(payload_size, '\0');
    bytes_read = recv(client_socket, &payload[0], payload_size, MSG_WAITALL);
    if (bytes_read != static_cast<ssize_t>(payload_size)) {
        std::cerr << "Failed to read payload" << std::endl;
        return;
    }
    
    std::cout << "Received - Header: " << header << std::endl;
    std::cout << "Received - Payload: " << payload << std::endl;
    
    // Call callback if set
    if (onDataReceived) {
        onDataReceived(header, payload);
    }
}