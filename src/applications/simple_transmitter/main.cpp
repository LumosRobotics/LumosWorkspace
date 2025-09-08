#include <iostream>
#include <string>
#include <vector>
#include "../../modules/tcp_client/tcp_client.h"

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    // Create TCP client
    TCPClient client(host, port);
    
    std::cout << "Simple TCP Transmitter (using tcp_client module)" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  list [name] - Send a list of integers (optional name)" << std::endl;
    std::cout << "  string [name] <value> - Send a string value (optional name)" << std::endl;
    std::cout << "  connect - Connect to server" << std::endl;
    std::cout << "  disconnect - Disconnect from server" << std::endl;
    std::cout << "  status - Show connection status" << std::endl;
    std::cout << "  quit - Exit" << std::endl;
    std::cout << std::endl;
    
    std::string command;
    while (true) {
        std::cout << "transmitter> ";
        std::getline(std::cin, command);
        
        if (command == "quit" || command == "exit") {
            break;
        }
        
        if (command == "connect") {
            if (client.connect()) {
                std::cout << "Connected to " << host << ":" << port << std::endl;
            } else {
                std::cout << "Failed to connect to " << host << ":" << port << std::endl;
            }
        }
        else if (command == "disconnect") {
            client.disconnect();
            std::cout << "Disconnected" << std::endl;
        }
        else if (command == "status") {
            std::cout << "Connection status: " << (client.isConnected() ? "Connected" : "Disconnected") << std::endl;
            std::cout << "Target: " << client.getHost() << ":" << client.getPort() << std::endl;
        }
        else if (command.substr(0, 4) == "list") {
            if (!client.isConnected()) {
                std::cout << "Not connected. Use 'connect' command first." << std::endl;
                continue;
            }
            
            std::vector<int> data = {1, 2, 3, 4, 5, 42, 100};
            std::string name;
            
            if (command.size() > 5) {
                name = command.substr(5);
            }
            
            if (client.sendIntList(data, name)) {
                if (!name.empty()) {
                    std::cout << "Sent integer list with name '" << name << "'" << std::endl;
                } else {
                    std::cout << "Sent integer list (random name will be assigned)" << std::endl;
                }
            } else {
                std::cout << "Failed to send message" << std::endl;
            }
        }
        else if (command.substr(0, 6) == "string") {
            if (!client.isConnected()) {
                std::cout << "Not connected. Use 'connect' command first." << std::endl;
                continue;
            }
            
            std::string name;
            std::string value = "hello world";  // default value
            
            if (command.size() > 7) {
                std::string rest = command.substr(7);
                size_t space_pos = rest.find(' ');
                
                if (space_pos != std::string::npos) {
                    // Both name and value provided
                    name = rest.substr(0, space_pos);
                    value = rest.substr(space_pos + 1);
                } else {
                    // Only value provided (no name)
                    value = rest;
                }
            }
            
            if (client.sendString(value, name)) {
                std::cout << "Sent string";
                if (!name.empty()) {
                    std::cout << " with name '" << name << "'";
                }
                std::cout << ": \"" << value << "\"" << std::endl;
            } else {
                std::cout << "Failed to send message" << std::endl;
            }
        }
        else {
            std::cout << "Unknown command. Available commands:" << std::endl;
            std::cout << "  connect, disconnect, status, list [name], string [name] <value>, quit" << std::endl;
        }
    }
    
    // Cleanup - disconnect if still connected
    if (client.isConnected()) {
        client.disconnect();
    }
    
    return 0;
}