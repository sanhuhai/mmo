#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class NetworkClient {
public:
    NetworkClient(const std::string& server_ip, uint16_t server_port)
        : server_ip_(server_ip)
        , server_port_(server_port)
        , socket_(INVALID_SOCKET) {
    }

    ~NetworkClient() {
        Disconnect();
    }

    bool Connect() {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }

        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_ == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            WSACleanup();
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port_);
        inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);

        if (connect(socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to server" << std::endl;
            closesocket(socket_);
            WSACleanup();
            return false;
        }

        std::cout << "Connected to server " << server_ip_ << ":" << server_port_ << std::endl;
        return true;
    }

    void Disconnect() {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
        WSACleanup();
    }

    bool SendLoginRequest(const std::string& account, const std::string& password) {
        std::string login_data = account + ":" + password;
        
        uint32_t msg_id = 1;
        uint32_t msg_len = static_cast<uint32_t>(login_data.size());

        std::vector<uint8_t> data;
        data.reserve(sizeof(uint32_t) * 2 + msg_len);

        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_id), 
            reinterpret_cast<const uint8_t*>(&msg_id) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_len), 
            reinterpret_cast<const uint8_t*>(&msg_len) + sizeof(uint32_t));
        data.insert(data.end(), login_data.begin(), login_data.end());

        return SendData(data);
    }

    bool SendHeartbeat() {
        int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        uint32_t msg_id = 100;
        uint32_t msg_len = sizeof(timestamp);

        std::vector<uint8_t> data;
        data.reserve(sizeof(uint32_t) * 2 + msg_len);

        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_id), 
            reinterpret_cast<const uint8_t*>(&msg_id) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_len), 
            reinterpret_cast<const uint8_t*>(&msg_len) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&timestamp), 
            reinterpret_cast<const uint8_t*>(&timestamp) + sizeof(timestamp));

        return SendData(data);
    }

    bool ReceiveData() {
        std::vector<uint8_t> header(sizeof(uint32_t) * 2);
        int received = 0;

        while (received < sizeof(uint32_t) * 2) {
            int bytes = recv(socket_, reinterpret_cast<char*>(header.data()) + received, 
                static_cast<int>(sizeof(uint32_t) * 2 - received), 0);
            if (bytes <= 0) {
                return false;
            }
            received += bytes;
        }

        uint32_t msg_id = 0;
        uint32_t msg_len = 0;
        std::memcpy(&msg_id, header.data(), sizeof(uint32_t));
        std::memcpy(&msg_len, header.data() + sizeof(uint32_t), sizeof(uint32_t));

        std::cout << "Received message: msg_id=" << msg_id << ", msg_len=" << msg_len << std::endl;

        if (msg_len > 0) {
            std::vector<uint8_t> data(msg_len);
            received = 0;

            while (static_cast<size_t>(received) < msg_len) {
                int bytes = recv(socket_, reinterpret_cast<char*>(data.data()) + received, 
                    static_cast<int>(msg_len - received), 0);
                if (bytes <= 0) {
                    return false;
                }
                received += bytes;
            }

            std::string data_str(data.begin(), data.end());
            std::cout << "Message data: " << data_str << std::endl;
        }

        return true;
    }

private:
    bool SendData(const std::vector<uint8_t>& data) {
        int sent = 0;
        while (static_cast<size_t>(sent) < data.size()) {
            int bytes = send(socket_, reinterpret_cast<const char*>(data.data()) + sent, 
                static_cast<int>(data.size() - sent), 0);
            if (bytes == SOCKET_ERROR) {
                std::cerr << "Failed to send data" << std::endl;
                return false;
            }
            sent += bytes;
        }
        return true;
    }

private:
    std::string server_ip_;
    uint16_t server_port_;
    SOCKET socket_;
};

int main() {
    NetworkClient client("127.0.0.1", 8888);

    if (!client.Connect()) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::cout << "Sending login request..." << std::endl;
    if (!client.SendLoginRequest("test_user", "test_password")) {
        std::cerr << "Failed to send login request" << std::endl;
        return 1;
    }

    std::cout << "Receiving response..." << std::endl;
    if (!client.ReceiveData()) {
        std::cerr << "Failed to receive response" << std::endl;
        return 1;
    }

    std::cout << "Sending heartbeat..." << std::endl;
    if (!client.SendHeartbeat()) {
        std::cerr << "Failed to send heartbeat" << std::endl;
        return 1;
    }

    std::cout << "Receiving heartbeat response..." << std::endl;
    if (!client.ReceiveData()) {
        std::cerr << "Failed to receive heartbeat response" << std::endl;
        return 1;
    }

    std::cout << "Test completed successfully!" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
