#include "proto/login.pb.h"
#include "proto/common.pb.h"


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

#ifdef _DEBUG
#pragma comment(lib, "libprotobufd.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#endif

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
        mmo::LoginRequest request;
        request.set_account(account);
        request.set_password(password);
        request.set_platform("test");
        request.set_version("1.0.0");
        request.set_device_id("test_device");

        std::string serialized;
        if (!request.SerializeToString(&serialized)) {
            std::cerr << "Failed to serialize login request" << std::endl;
            return false;
        }

        uint32_t msg_id = 1;
        uint32_t msg_len = static_cast<uint32_t>(serialized.size());

        std::vector<uint8_t> data;
        data.reserve(sizeof(uint32_t) * 2 + msg_len);

        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_id), 
            reinterpret_cast<const uint8_t*>(&msg_id) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_len), 
            reinterpret_cast<const uint8_t*>(&msg_len) + sizeof(uint32_t));
        data.insert(data.end(), serialized.begin(), serialized.end());

        std::cout << "Sending login request: account=" << account << ", size=" << data.size() << std::endl;
        return SendData(data);
    }

    bool SendHeartbeat() {
        mmo::Heartbeat request;
        request.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());

        std::string serialized;
        if (!request.SerializeToString(&serialized)) {
            std::cerr << "Failed to serialize heartbeat" << std::endl;
            return false;
        }

        uint32_t msg_id = 100;
        uint32_t msg_len = static_cast<uint32_t>(serialized.size());

        std::vector<uint8_t> data;
        data.reserve(sizeof(uint32_t) * 2 + msg_len);

        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_id), 
            reinterpret_cast<const uint8_t*>(&msg_id) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_len), 
            reinterpret_cast<const uint8_t*>(&msg_len) + sizeof(uint32_t));
        data.insert(data.end(), serialized.begin(), serialized.end());

        std::cout << "Sending heartbeat, size=" << data.size() << std::endl;
        return SendData(data);
    }

    bool ReceiveLoginResponse() {
        std::vector<uint8_t> header(sizeof(uint32_t) * 2);
        int received = 0;

        while (received < sizeof(uint32_t) * 2) {
            int bytes = recv(socket_, reinterpret_cast<char*>(header.data()) + received, 
                static_cast<int>(sizeof(uint32_t) * 2 - received), 0);
            if (bytes <= 0) {
                std::cerr << "Failed to receive header" << std::endl;
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
                    std::cerr << "Failed to receive data" << std::endl;
                    return false;
                }
                received += bytes;
            }

            if (msg_id == 1) {
                mmo::LoginResponse response;
                if (response.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
                    std::cout << "Login response: code=" << response.result().code() 
                              << ", message=" << response.result().message()
                              << ", player_id=" << response.player_id()
                              << ", token=" << response.token() << std::endl;
                } else {
                    std::cerr << "Failed to parse login response" << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    bool ReceiveHeartbeatResponse() {
        std::vector<uint8_t> header(sizeof(uint32_t) * 2);
        int received = 0;

        while (received < sizeof(uint32_t) * 2) {
            int bytes = recv(socket_, reinterpret_cast<char*>(header.data()) + received, 
                static_cast<int>(sizeof(uint32_t) * 2 - received), 0);
            if (bytes <= 0) {
                std::cerr << "Failed to receive header" << std::endl;
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
                    std::cerr << "Failed to receive data" << std::endl;
                    return false;
                }
                received += bytes;
            }

            if (msg_id == 100) {
                mmo::Heartbeat response;
                if (response.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
                    std::cout << "Heartbeat response: timestamp=" << response.timestamp() << std::endl;
                } else {
                    std::cerr << "Failed to parse heartbeat response" << std::endl;
                    return false;
                }
            }
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
        std::cout << "Sent " << sent << " bytes" << std::endl;
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
    if (!client.ReceiveLoginResponse()) {
        std::cerr << "Failed to receive response" << std::endl;
        return 1;
    }

    std::cout << "Sending heartbeat..." << std::endl;
    if (!client.SendHeartbeat()) {
        std::cerr << "Failed to send heartbeat" << std::endl;
        return 1;
    }

    std::cout << "Receiving heartbeat response..." << std::endl;
    if (!client.ReceiveHeartbeatResponse()) {
        std::cerr << "Failed to receive heartbeat response" << std::endl;
        return 1;
    }

    std::cout << "Test completed successfully!" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
