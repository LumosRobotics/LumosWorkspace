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
    std::cout << "  list <name> - Send a list of integers as <name>" << std::endl;
    std::cout << "  string <name> <value> - Send a string value as <name>" << std::endl;
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
            std::string name = "test_list";
            if (command.size() > 5) {
                name = command.substr(5);
            }
            
            std::string header = "{\"type\": \"int_list\", \"name\": \"" + name + "\"}";
            std::string payload = "[1, 2, 3, 4, 5, 42, 100]";
            
            if (sendMessage(host, port, header, payload)) {
                std::cout << "Sent integer list '" << name << "'" << std::endl;
            } else {
                std::cout << "Failed to send message" << std::endl;
            }
        }
        else if (command.substr(0, 6) == "string") {
            std::string rest = command.substr(7);
            size_t space_pos = rest.find(' ');
            
            std::string name = "test_string";
            std::string value = "hello world";
            
            if (space_pos != std::string::npos) {
                name = rest.substr(0, space_pos);
                value = rest.substr(space_pos + 1);
            }
            
            std::string header = "{\"type\": \"string\", \"name\": \"" + name + "\"}";
            std::string payload = "\"" + value + "\"";
            
            if (sendMessage(host, port, header, payload)) {
                std::cout << "Sent string '" << name << "' = '" << value << "'" << std::endl;
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