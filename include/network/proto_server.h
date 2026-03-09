#pragma once

#include "network/tcp_server.h"
#include "network/proto_connection.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mmo {

class ProtoServer {
public:
    using Ptr = std::shared_ptr<ProtoServer>;
    using MessageHandler = std::function<void(ProtoConnection::Ptr, MessageType, MessageCodec::MessagePtr)>;
    using ConnectHandler = std::function<void(ProtoConnection::Ptr)>;
    using DisconnectHandler = std::function<void(ProtoConnection::Ptr)>;

    ProtoServer(uint16_t port, size_t thread_count = 4)
        : tcp_server_(std::make_unique<TcpServer>(port, thread_count)) {
    }

    ~ProtoServer() {
        Stop();
    }

    bool Start() {
        if (!MessageCodec::Instance().Initialize()) {
            LOG_ERROR("Failed to initialize MessageCodec");
            return false;
        }

        tcp_server_->SetAcceptCallback([this](Connection::Ptr conn) {
            HandleAccept(conn);
        });

        tcp_server_->SetMessageCallback([this](Connection::Ptr conn, const std::vector<uint8_t>& data) {
        });

        return tcp_server_->Start();
    }

    void Stop() {
        tcp_server_->Stop();
        
        std::lock_guard<std::mutex> lock(mutex_);
        proto_connections_.clear();
    }

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

    void SetConnectHandler(ConnectHandler handler) {
        connect_handler_ = handler;
    }

    void SetDisconnectHandler(DisconnectHandler handler) {
        disconnect_handler_ = handler;
    }

    void Broadcast(MessageType type, const google::protobuf::Message& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : proto_connections_) {
            if (pair.second->IsConnected()) {
                pair.second->Send(type, message);
            }
        }
    }

    void SendTo(uint32_t connection_id, MessageType type, const google::protobuf::Message& message) {
        auto conn = GetConnection(connection_id);
        if (conn) {
            conn->Send(type, message);
        }
    }

    ProtoConnection::Ptr GetConnection(uint32_t connection_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = proto_connections_.find(connection_id);
        if (it != proto_connections_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void KickConnection(uint32_t connection_id) {
        auto conn = GetConnection(connection_id);
        if (conn) {
            conn->Close();
            
            std::lock_guard<std::mutex> lock(mutex_);
            proto_connections_.erase(connection_id);
        }
    }

    size_t GetConnectionCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return proto_connections_.size();
    }

    uint16_t GetPort() const { return tcp_server_->GetPort(); }
    bool IsRunning() const { return tcp_server_->IsRunning(); }

private:
    void HandleAccept(Connection::Ptr conn) {
        auto proto_conn = std::make_shared<ProtoConnection>(conn);
        
        proto_conn->SetMessageHandler([this](ProtoConnection::Ptr pc, MessageType type, MessageCodec::MessagePtr msg) {
            if (message_handler_) {
                message_handler_(pc, type, msg);
            }
        });

        proto_conn->Start();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            proto_connections_[conn->GetConnectionId()] = proto_conn;
        }

        LOG_INFO("ProtoConnection accepted: {} from {}", 
                 conn->GetConnectionId(), conn->GetRemoteAddress());

        if (connect_handler_) {
            connect_handler_(proto_conn);
        }

        conn->SetErrorHandler([this, proto_conn](Connection::Ptr c, const std::string& error) {
            HandleDisconnect(proto_conn);
        });
    }

    void HandleDisconnect(ProtoConnection::Ptr proto_conn) {
        uint32_t conn_id = proto_conn->GetConnectionId();
        
        LOG_INFO("ProtoConnection disconnected: {}", conn_id);

        if (disconnect_handler_) {
            disconnect_handler_(proto_conn);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        proto_connections_.erase(conn_id);
    }

    std::unique_ptr<TcpServer> tcp_server_;
    std::unordered_map<uint32_t, ProtoConnection::Ptr> proto_connections_;
    std::mutex mutex_;

    MessageHandler message_handler_;
    ConnectHandler connect_handler_;
    DisconnectHandler disconnect_handler_;
};

}
