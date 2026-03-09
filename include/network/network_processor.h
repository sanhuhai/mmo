#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>
#include "message_queue.h"
#include "network.pb.h"
#include "login.pb.h"
#include "player.pb.h"
#include "game.pb.h"

namespace mmo {

struct NetworkPacket {
    uint32_t msg_id;
    uint64_t session_id;
    std::vector<uint8_t> data;
    int64_t timestamp;

    NetworkPacket()
        : msg_id(0)
        , session_id(0)
        , timestamp(0) {
    }

    NetworkPacket(uint32_t id, uint64_t sid, const std::vector<uint8_t>& d, int64_t ts = 0)
        : msg_id(id)
        , session_id(sid)
        , data(d)
        , timestamp(ts) {
    }

    NetworkPacket(uint32_t id, uint64_t sid, std::vector<uint8_t>&& d, int64_t ts = 0)
        : msg_id(id)
        , session_id(sid)
        , data(std::move(d))
        , timestamp(ts) {
    }

    size_t Size() const {
        return data.size();
    }

    bool IsValid() const {
        return msg_id > 0 && !data.empty();
    }
};

// Message handler base class
class MessageHandlerBase {
public:
    virtual ~MessageHandlerBase() = default;
    virtual void Handle(const std::vector<uint8_t>& data, uint64_t session_id) = 0;
};

// Template message handler
template<typename MessageType>
class TypedMessageHandler : public MessageHandlerBase {
public:
    using HandlerFunction = std::function<void(const MessageType&, uint64_t)>;

    TypedMessageHandler(HandlerFunction handler) : handler_(handler) {}

    void Handle(const std::vector<uint8_t>& data, uint64_t session_id) override {
        MessageType message;
        if (message.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
            handler_(message, session_id);
        }
    }

private:
    HandlerFunction handler_;
};

class NetworkMessageProcessor {
public:
    NetworkMessageProcessor();
    ~NetworkMessageProcessor();

    bool Initialize();
    void Shutdown();

    bool Start();
    void Stop();

    bool ProcessPacket(const NetworkPacket& packet);

    template<typename MessageType>
    void RegisterHandler(uint32_t msg_id, std::function<void(const MessageType&, uint64_t)> handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_[msg_id] = std::make_shared<TypedMessageHandler<MessageType>>(handler);
    }

    void UnregisterHandler(uint32_t msg_id);

    size_t GetQueueSize() const;
    bool IsQueueFull() const;

private:
    void ProcessMessage(const NetworkPacket& packet);
    void HandleError(const std::string& error);

    MessageQueue<NetworkPacket> message_queue_;
    std::unordered_map<uint32_t, std::shared_ptr<MessageHandlerBase>> handlers_;
    mutable std::mutex handlers_mutex_;
    std::atomic<bool> running_;
};

}
