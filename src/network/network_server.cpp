#include "network/network_server.h"
#include "core/logger.h"
#include <chrono>
#include <cstring>

namespace mmo {

NetworkServer::NetworkServer(uint16_t port)
    : port_(port)
    , message_processor_()
    , running_(false) {
}

NetworkServer::~NetworkServer() {
    Shutdown();
}

bool NetworkServer::Initialize() {
    if (!message_processor_.Initialize()) {
        LOG_ERROR("Failed to initialize message processor");
        return false;
    }

    tcp_server_ = std::make_unique<TcpServer>(port_);

    tcp_server_->SetAcceptCallback([this](mmo::Connection::Ptr conn) {
        std::string remote = conn->GetRemoteAddress();
        size_t colon_pos = remote.find(':');
        std::string ip = (colon_pos != std::string::npos) ? remote.substr(0, colon_pos) : remote;
        uint16_t port = (colon_pos != std::string::npos) ? std::stoi(remote.substr(colon_pos + 1)) : 0;
        OnClientConnected(conn->GetConnectionId(), ip, port);
    });

    tcp_server_->SetMessageCallback([this](mmo::Connection::Ptr conn, const std::vector<uint8_t>& data) {
        OnMessageReceived(conn->GetConnectionId(), data);
    });

    LOG_INFO("Network server initialized on port {}", port_);
    return true;
}

void NetworkServer::Shutdown() {
    Stop();
    if (tcp_server_) {
        tcp_server_->Stop();
    }
    message_processor_.Shutdown();
    LOG_INFO("Network server shutdown");
}

bool NetworkServer::Start() {
    if (running_) {
        LOG_WARN("Network server already running");
        return false;
    }

    RegisterHandler<mmo::LoginRequest>(1, [this](const mmo::LoginRequest& request, uint64_t session_id) {
        HandleLoginRequest(request, session_id);
    });

    RegisterHandler<mmo::LogoutRequest>(2, [this](const mmo::LogoutRequest& request, uint64_t session_id) {
        HandleLogoutRequest(request, session_id);
    });

    //RegisterHandler<mmo::ClientConnect>(3,[this](const mmo::ClientConnect& request,))

    RegisterHandler<mmo::Heartbeat>(100, [this](const mmo::Heartbeat& heartbeat, uint64_t session_id) {
        HandleHeartbeat(heartbeat, session_id);
    });

    if (!message_processor_.Start()) {
        LOG_ERROR("Failed to start message processor");
        return false;
    }

    if (!tcp_server_->Start()) {
        LOG_ERROR("Failed to start TCP server");
        return false;
    }

    running_ = true;
    LOG_INFO("Network server started on port {}", port_);
    return true;
}

void NetworkServer::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (tcp_server_) {
        tcp_server_->Stop();
    }

    message_processor_.Stop();

    LOG_INFO("Network server stopped");
}

void NetworkServer::Update(int64_t delta_ms) {
    if (!running_) {
        return;
    }

    if (tcp_server_) {
        // TcpServer使用异步IO，不需要手动Update
        // 这里可以添加其他需要定期更新的逻辑
    }
}

void NetworkServer::OnClientConnected(uint64_t session_id, const std::string& ip, uint16_t port) {
    LOG_INFO("Client connected: session_id={}, ip={}:{}", session_id, ip, port);
    LOG_WARN("Client connected: session_id={}, ip={}:{}", session_id, ip, port);

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session_id] = ip;
    }

    mmo::NetworkMessage connect_msg;
    connect_msg.set_msg_id(200);
    connect_msg.set_session_id(session_id);
    connect_msg.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());

    message_processor_.ProcessPacket(NetworkPacket(200, session_id, 
        std::vector<uint8_t>(connect_msg.ByteSizeLong()), connect_msg.timestamp()));
}

void NetworkServer::OnClientDisconnected(uint64_t session_id) {
    LOG_INFO("Client disconnected: session_id={}", session_id);

    std::string ip;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(session_id);
        if (it != sessions_.end()) {
            ip = it->second;
            sessions_.erase(it);
        }
    }

    mmo::NetworkMessage disconnect_msg;
    disconnect_msg.set_msg_id(201);
    disconnect_msg.set_session_id(session_id);
    disconnect_msg.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());

    message_processor_.ProcessPacket(NetworkPacket(201, session_id, 
        std::vector<uint8_t>(disconnect_msg.ByteSizeLong()), disconnect_msg.timestamp()));
}

void NetworkServer::OnMessageReceived(uint64_t session_id, const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint32_t) * 2) {
        LOG_ERROR("Invalid message size: {}", data.size());
        return;
    }

    uint32_t msg_id = 0;
    uint32_t msg_len = 0;

    std::memcpy(&msg_id, data.data(), sizeof(uint32_t));
    std::memcpy(&msg_len, data.data() + sizeof(uint32_t), sizeof(uint32_t));

    if (msg_len != data.size() - sizeof(uint32_t) * 2) {
        LOG_ERROR("Message length mismatch: expected={}, actual={}", msg_len, data.size() - sizeof(uint32_t) * 2);
        return;
    }

    std::vector<uint8_t> msg_data(data.begin() + sizeof(uint32_t) * 2, data.end());

    NetworkPacket packet(msg_id, session_id, msg_data, 
        std::chrono::system_clock::now().time_since_epoch().count());

    message_processor_.ProcessPacket(packet);
}

void NetworkServer::HandleLoginRequest(const mmo::LoginRequest& request, uint64_t session_id) {
    LOG_INFO("Login request: account={}, session_id={}", request.account(), session_id);

    mmo::LoginResponse response;
    response.mutable_result()->set_code(0);
    response.mutable_result()->set_message("Login successful");
    response.set_player_id(12345);
    response.set_token("test_token_12345");
    response.set_server_time(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    //message_processor_.Push(response);
    SendResponse(session_id, 1, response);
}

void NetworkServer::HandleLogoutRequest(const mmo::LogoutRequest& request, uint64_t session_id) {
    LOG_INFO("Logout request: player_id={}, session_id={}", request.player_id(), session_id);

    mmo::LogoutResponse response;
    response.mutable_result()->set_code(0);
    response.mutable_result()->set_message("Logout successful");

    SendResponse(session_id, 2, response);
}

void NetworkServer::HandleHeartbeat(const mmo::Heartbeat& heartbeat, uint64_t session_id) {
    LOG_DEBUG("Heartbeat: session_id={}, timestamp={}", session_id, heartbeat.timestamp());

    mmo::Heartbeat response;
    response.set_timestamp(static_cast<int64_t>(std::chrono::system_clock::now().time_since_epoch().count()));

    SendResponse(session_id, 100, response);
}

void NetworkServer::SendResponse(uint64_t session_id, uint32_t msg_id, const google::protobuf::Message& message) {
    if (!tcp_server_) {
        return;
    }

    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        LOG_ERROR("Failed to serialize message: msg_id={}", msg_id);
        return;
    }

    uint32_t msg_len = static_cast<uint32_t>(serialized.size());

    std::vector<uint8_t> data;
    data.reserve(sizeof(uint32_t) * 2 + msg_len);

    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_id), 
        reinterpret_cast<const uint8_t*>(&msg_id) + sizeof(uint32_t));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&msg_len), 
        reinterpret_cast<const uint8_t*>(&msg_len) + sizeof(uint32_t));
    data.insert(data.end(), serialized.begin(), serialized.end());

    tcp_server_->SendTo(session_id, data);
}

}
