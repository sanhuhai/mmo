#pragma once

#include "network/tcp_server.h"
#include "network/network_processor.h"
#include "network.pb.h"
#include "login.pb.h"
#include "player.pb.h"
#include "game.pb.h"
#include <unordered_map>
#include <memory>

namespace mmo {

class NetworkServer {
public:
    NetworkServer(uint16_t port);
    ~NetworkServer();

    bool Initialize();
    void Shutdown();

    bool Start();
    void Stop();

    bool IsRunning() const { return running_; }

    void Update(int64_t delta_ms);

    template<typename MessageType>
    void RegisterHandler(uint32_t msg_id, std::function<void(const MessageType&, uint64_t)> handler) {
        message_processor_.RegisterHandler(msg_id, handler);
    }

private:
    void OnClientConnected(uint64_t session_id, const std::string& ip, uint16_t port);
    void OnClientDisconnected(uint64_t session_id);
    void OnMessageReceived(uint64_t session_id, const std::vector<uint8_t>& data);
    void HandleLoginRequest(const mmo::LoginRequest& request, uint64_t session_id);
    void HandleLogoutRequest(const mmo::LogoutRequest& request, uint64_t session_id);
    void HandleHeartbeat(const mmo::Heartbeat& heartbeat, uint64_t session_id);
    void SendResponse(uint64_t session_id, uint32_t msg_id, const google::protobuf::Message& message);

private:
    uint16_t port_;
    std::unique_ptr<TcpServer> tcp_server_;
    NetworkMessageProcessor message_processor_;
    std::atomic<bool> running_;
    std::unordered_map<uint64_t, std::string> sessions_;
    std::mutex sessions_mutex_;
};

}
