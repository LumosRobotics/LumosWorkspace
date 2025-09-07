#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

bool sendMessage(const std::string& host, int port, const std::string& header, const std::string& payload) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        close(client_socket);
        return false;
    }
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(client_socket);
        return false;
    }
    
    // Send header size (network byte order)
    uint32_t header_size = htonl(header.size());
    if (send(client_socket, &header_size, sizeof(header_size), 0) < 0) {
        std::cerr << "Failed to send header size" << std::endl;
        close(client_socket);
        return false;
    }
    
    // Send header
    if (send(client_socket, header.c_str(), header.size(), 0) < 0) {
        std::cerr << "Failed to send header" << std::endl;
        close(client_socket);
        return false;
    }
    
    // Send payload size (network byte order)
    uint32_t payload_size = htonl(payload.size());
    if (send(client_socket, &payload_size, sizeof(payload_size), 0) < 0) {
        std::cerr << "Failed to send payload size" << std::endl;
        close(client_socket);
        return false;
    }
    
    // Send payload
    if (send(client_socket, payload.c_str(), payload.size(), 0) < 0) {
        std::cerr << "Failed to send payload" << std::endl;
        close(client_socket);
        return false;
    }
    
    close(client_socket);
    return true;
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    std::cout << "Simple TCP Transmitter" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  list [name] - Send a list of integers (optional name)" << std::endl;
    std::cout << "  string [name] <value> - Send a string value (optional name)" << std::endl;
    std::cout << "  quit - Exit" << std::endl;
    std::cout << std::endl;
    
    std::string command;
    while (true) {
        std::cout << "transmitter> ";
        std::getline(std::cin, command);
        
        if (command == "quit" || command == "exit") {
            break;
        }
        
        if (command.substr(0, 4) == "list") {
            std::string header;
            if (command.size() > 5) {
                // Name provided
                std::string name = command.substr(5);
                header = "{\"type\": \"int_list\", \"name\": \"" + name + "\"}";
            } else {
                // No name provided - let receiver generate random name
                header = "{\"type\": \"int_list\"}";
            }
            
            std::string payload = "[1, 2, 3, 4, 5, 42, 100]";
            
            if (sendMessage(host, port, header, payload)) {
                if (command.size() > 5) {
                    std::cout << "Sent integer list with name '" << command.substr(5) << "'" << std::endl;
                } else {
                    std::cout << "Sent integer list (random name will be assigned)" << std::endl;
                }
            } else {
                std::cout << "Failed to send message" << std::endl;
            }
        }
        else if (command.substr(0, 6) == "string") {
            std::string header;
            std::string payload;
            
            if (command.size() > 7) {
                std::string rest = command.substr(7);
                size_t space_pos = rest.find(' ');
                
                if (space_pos != std::string::npos) {
                    // Both name and value provided
                    std::string name = rest.substr(0, space_pos);
                    std::string value = rest.substr(space_pos + 1);
                    header = "{\"type\": \"string\", \"name\": \"" + name + "\"}";
                    payload = "\"" + value + "\"";
                } else {
                    // Only value provided, no name
                    std::string value = rest;
                    header = "{\"type\": \"string\"}";
                    payload = "\"" + value + "\"";
                }
            } else {
                // No name or value provided
                header = "{\"type\": \"string\"}";
                payload = "\"hello world\"";
            }
            
            if (sendMessage(host, port, header, payload)) {
                std::cout << "Sent string successfully" << std::endl;
            } else {
                std::cout << "Failed to send message" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command. Use 'list', 'string', or 'quit'" << std::endl;
        }
    }
    
    return 0;
}