#pragma once

#include <string>
#include <vector>

class TCPClient {
private:
    std::string host;
    int port;
    int socket_fd;
    bool connected;
    
    bool createSocket();
    void closeSocket();
    
public:
    TCPClient(const std::string& host = "127.0.0.1", int port = 8080);
    ~TCPClient();
    
    // Connect to server
    bool connect();
    
    // Disconnect from server
    void disconnect();
    
    // Check if connected
    bool isConnected() const;
    
    // Send message with header and payload
    bool sendMessage(const std::string& header, const std::string& payload);
    
    // Convenience methods for common message types
    bool sendIntList(const std::vector<int>& data, const std::string& name = "");
    bool sendString(const std::string& data, const std::string& name = "");
    bool sendRawData(const std::string& header_json, const std::string& payload);
    
    // Getters/setters
    std::string getHost() const;
    int getPort() const;
    void setTarget(const std::string& host, int port);
};