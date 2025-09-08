#include "tcp_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>

TCPClient::TCPClient(const std::string& host, int port) 
    : host(host), port(port), socket_fd(-1), connected(false) {
}

TCPClient::~TCPClient() {
    disconnect();
}

bool TCPClient::createSocket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    return true;
}

void TCPClient::closeSocket() {
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
    connected = false;
}

bool TCPClient::connect() {
    if (connected) {
        return true;
    }
    
    if (!createSocket()) {
        return false;
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        closeSocket();
        return false;
    }
    
    if (::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed to " << host << ":" << port << std::endl;
        closeSocket();
        return false;
    }
    
    connected = true;
    return true;
}

void TCPClient::disconnect() {
    closeSocket();
}

bool TCPClient::isConnected() const {
    return connected;
}

bool TCPClient::sendMessage(const std::string& header, const std::string& payload) {
    if (!connected) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    // Send header size (network byte order)
    uint32_t header_size = htonl(header.size());
    if (send(socket_fd, &header_size, sizeof(header_size), 0) < 0) {
        std::cerr << "Failed to send header size" << std::endl;
        return false;
    }
    
    // Send header
    if (send(socket_fd, header.c_str(), header.size(), 0) < 0) {
        std::cerr << "Failed to send header" << std::endl;
        return false;
    }
    
    // Send payload size (network byte order)
    uint32_t payload_size = htonl(payload.size());
    if (send(socket_fd, &payload_size, sizeof(payload_size), 0) < 0) {
        std::cerr << "Failed to send payload size" << std::endl;
        return false;
    }
    
    // Send payload
    if (send(socket_fd, payload.c_str(), payload.size(), 0) < 0) {
        std::cerr << "Failed to send payload" << std::endl;
        return false;
    }
    
    return true;
}

bool TCPClient::sendIntList(const std::vector<int>& data, const std::string& name) {
    std::string header;
    if (name.empty()) {
        header = "{\"type\": \"int_list\"}";
    } else {
        header = "{\"type\": \"int_list\", \"name\": \"" + name + "\"}";
    }
    
    std::ostringstream payload_stream;
    payload_stream << "[";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) payload_stream << ", ";
        payload_stream << data[i];
    }
    payload_stream << "]";
    
    return sendMessage(header, payload_stream.str());
}

bool TCPClient::sendString(const std::string& data, const std::string& name) {
    std::string header;
    if (name.empty()) {
        header = "{\"type\": \"string\"}";
    } else {
        header = "{\"type\": \"string\", \"name\": \"" + name + "\"}";
    }
    
    std::string payload = "\"" + data + "\"";
    return sendMessage(header, payload);
}

bool TCPClient::sendRawData(const std::string& header_json, const std::string& payload) {
    return sendMessage(header_json, payload);
}

std::string TCPClient::getHost() const {
    return host;
}

int TCPClient::getPort() const {
    return port;
}

void TCPClient::setTarget(const std::string& host, int port) {
    if (connected) {
        disconnect();
    }
    this->host = host;
    this->port = port;
}