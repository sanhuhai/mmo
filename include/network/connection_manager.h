#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>

#include "network/connection.h"

namespace mmo {

class ConnectionManager {
public:
    using Ptr = std::shared_ptr<ConnectionManager>;
    using ConnectionCallback = std::function<void(Connection::Ptr)>;
    using MessageCallback = std::function<void(Connection::Ptr, const std::vector<uint8_t>&)>;

    ConnectionManager() = default;
    ~ConnectionManager() = default;

    void AddConnection(Connection::Ptr conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = next_connection_id_++;
        conn->SetConnectionId(id);
        connections_[id] = conn;
        
        LOG_DEBUG("Connection added: {} (total: {})", id, connections_.size());
        
        if (on_connection_) {
            on_connection_(conn);
        }
    }

    void RemoveConnection(Connection::Ptr conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = conn->GetConnectionId();
        auto it = connections_.find(id);
        if (it != connections_.end()) {
            connections_.erase(it);
            LOG_DEBUG("Connection removed: {} (total: {})", id, connections_.size());
        }
    }

    void RemoveConnection(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end()) {
            connections_.erase(it);
            LOG_DEBUG("Connection removed: {} (total: {})", id, connections_.size());
        }
    }

    Connection::Ptr GetConnection(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Broadcast(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            if (pair.second->IsConnected()) {
                pair.second->Send(data);
            }
        }
    }

    void Broadcast(const uint8_t* data, size_t size) {
        std::vector<uint8_t> buffer(data, data + size);
        Broadcast(buffer);
    }

    void BroadcastExcept(uint32_t except_id, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            if (pair.first != except_id && pair.second->IsConnected()) {
                pair.second->Send(data);
            }
        }
    }

    size_t GetConnectionCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }

    void SetConnectionCallback(ConnectionCallback callback) {
        on_connection_ = callback;
    }

    void SetMessageCallback(MessageCallback callback) {
        on_message_ = callback;
    }

    void CloseAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            pair.second->Close();
        }
        connections_.clear();
        LOG_INFO("All connections closed");
    }

    void Cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            if (!it->second->IsConnected()) {
                it = connections_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::unordered_map<uint32_t, Connection::Ptr> connections_;
    std::mutex mutex_;
    uint32_t next_connection_id_ = 1;

    ConnectionCallback on_connection_;
    MessageCallback on_message_;
};

}
