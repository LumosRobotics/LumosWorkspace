#include <gtest/gtest.h>
#include "../tcp_client.h"
#include "../../tcp_server/tcp_server.h"
#include <thread>
#include <chrono>
#include <atomic>

class TCPClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create server and client for testing
        server = std::make_unique<TCPServer>(8081); // Use different port to avoid conflicts
        client = std::make_unique<TCPClient>("127.0.0.1", 8081);
        
        // Start server
        ASSERT_TRUE(server->start());
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (client && client->isConnected()) {
            client->disconnect();
        }
        if (server && server->isRunning()) {
            server->stop();
        }
    }

    std::unique_ptr<TCPServer> server;
    std::unique_ptr<TCPClient> client;
};

TEST_F(TCPClientTest, BasicConnectionTest) {
    EXPECT_FALSE(client->isConnected());
    
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->isConnected());
    
    client->disconnect();
    EXPECT_FALSE(client->isConnected());
}

TEST_F(TCPClientTest, GettersAndSetters) {
    EXPECT_EQ(client->getHost(), "127.0.0.1");
    EXPECT_EQ(client->getPort(), 8081);
    
    client->setTarget("192.168.1.1", 9090);
    EXPECT_EQ(client->getHost(), "192.168.1.1");
    EXPECT_EQ(client->getPort(), 9090);
}

TEST_F(TCPClientTest, SendMessageWithoutConnection) {
    EXPECT_FALSE(client->isConnected());
    
    // Should fail to send without connection
    EXPECT_FALSE(client->sendMessage("{\"type\": \"test\"}", "test payload"));
}

TEST_F(TCPClientTest, SendBasicMessage) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    // Set up server callback
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::string test_header = "{\"type\": \"test_message\"}";
    std::string test_payload = "Hello from client!";
    
    EXPECT_TRUE(client->sendMessage(test_header, test_payload));
    
    // Give server time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, test_header);
    EXPECT_EQ(received_payload, test_payload);
}

TEST_F(TCPClientTest, SendIntListWithName) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::vector<int> test_data = {1, 2, 3, 42, 100};
    std::string test_name = "test_list";
    
    EXPECT_TRUE(client->sendIntList(test_data, test_name));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, "{\"type\": \"int_list\", \"name\": \"test_list\"}");
    EXPECT_EQ(received_payload, "[1, 2, 3, 42, 100]");
}

TEST_F(TCPClientTest, SendIntListWithoutName) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::vector<int> test_data = {5, 10, 15};
    
    EXPECT_TRUE(client->sendIntList(test_data));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, "{\"type\": \"int_list\"}");
    EXPECT_EQ(received_payload, "[5, 10, 15]");
}

TEST_F(TCPClientTest, SendStringWithName) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::string test_string = "Hello World";
    std::string test_name = "greeting";
    
    EXPECT_TRUE(client->sendString(test_string, test_name));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, "{\"type\": \"string\", \"name\": \"greeting\"}");
    EXPECT_EQ(received_payload, "\"Hello World\"");
}

TEST_F(TCPClientTest, SendStringWithoutName) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::string test_string = "anonymous message";
    
    EXPECT_TRUE(client->sendString(test_string));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, "{\"type\": \"string\"}");
    EXPECT_EQ(received_payload, "\"anonymous message\"");
}

TEST_F(TCPClientTest, SendRawData) {
    std::string received_header;
    std::string received_payload;
    bool callback_called = false;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_header = header;
        received_payload = payload;
        callback_called = true;
    };
    
    EXPECT_TRUE(client->connect());
    
    std::string custom_header = "{\"type\": \"custom\", \"format\": \"binary\"}";
    std::string custom_payload = "\\x01\\x02\\x03\\x04";
    
    EXPECT_TRUE(client->sendRawData(custom_header, custom_payload));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_header, custom_header);
    EXPECT_EQ(received_payload, custom_payload);
}

TEST_F(TCPClientTest, MultipleMessages) {
    std::atomic<int> message_count(0);
    std::vector<std::string> received_headers;
    std::vector<std::string> received_payloads;
    
    server->onDataReceived = [&](const std::string& header, const std::string& payload) {
        received_headers.push_back(header);
        received_payloads.push_back(payload);
        message_count++;
    };
    
    // Send multiple messages with separate connections (since server closes after each)
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->sendString("Message 1", "msg1"));
    client->disconnect();
    
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->sendIntList({1, 2, 3}, "list1"));
    client->disconnect();
    
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->sendString("Message 2", "msg2"));
    client->disconnect();
    
    // Wait for all messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_EQ(message_count.load(), 3);
    EXPECT_EQ(received_headers.size(), 3u);
    EXPECT_EQ(received_payloads.size(), 3u);
    
    // Check first message
    EXPECT_EQ(received_headers[0], "{\"type\": \"string\", \"name\": \"msg1\"}");
    EXPECT_EQ(received_payloads[0], "\"Message 1\"");
    
    // Check second message
    EXPECT_EQ(received_headers[1], "{\"type\": \"int_list\", \"name\": \"list1\"}");
    EXPECT_EQ(received_payloads[1], "[1, 2, 3]");
    
    // Check third message
    EXPECT_EQ(received_headers[2], "{\"type\": \"string\", \"name\": \"msg2\"}");
    EXPECT_EQ(received_payloads[2], "\"Message 2\"");
}

TEST_F(TCPClientTest, ReconnectionTest) {
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->isConnected());
    
    client->disconnect();
    EXPECT_FALSE(client->isConnected());
    
    // Should be able to reconnect
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->isConnected());
}