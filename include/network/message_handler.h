#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include "core/logger.h"

namespace mmo {

template<typename MessageType>
class MessageHandler {
public:
    using HandlerFunction = std::function<void(const MessageType&, uint64_t)>;

    MessageHandler() = default;
    ~MessageHandler() = default;

    void RegisterHandler(uint32_t msg_id, HandlerFunction handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_[msg_id] = handler;
    }

    void UnregisterHandler(uint32_t msg_id) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.erase(msg_id);
    }

    bool HandleMessage(uint32_t msg_id, const MessageType& message, uint64_t session_id = 0) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find(msg_id);
        if (it != handlers_.end()) {
            try {
                it->second(message, session_id);
                return true;
            } catch (const std::exception& e) {
                LOG_ERROR("Message handler error for msg_id {}: {}", msg_id, e.what());
                return false;
            }
        }
        LOG_WARN("No handler registered for msg_id {}", msg_id);
        return false;
    }

    bool HasHandler(uint32_t msg_id) const {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        return handlers_.find(msg_id) != handlers_.end();
    }

    size_t HandlerCount() const {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        return handlers_.size();
    }

private:
    std::unordered_map<uint32_t, HandlerFunction> handlers_;
    mutable std::mutex handlers_mutex_;
};

template<typename MessageType>
class MessageDispatcher {
public:
    using MessageHandlerType = MessageHandler<MessageType>;

    MessageDispatcher() = default;
    ~MessageDispatcher() = default;

    void RegisterHandler(uint32_t msg_id, typename MessageHandlerType::HandlerFunction handler) {
        message_handler_.RegisterHandler(msg_id, handler);
        LOG_INFO("Registered handler for msg_id {}", msg_id);
    }

    void UnregisterHandler(uint32_t msg_id) {
        message_handler_.UnregisterHandler(msg_id);
        LOG_INFO("Unregistered handler for msg_id {}", msg_id);
    }

    bool Dispatch(uint32_t msg_id, const MessageType& message, uint64_t session_id = 0) {
        return message_handler_.HandleMessage(msg_id, message, session_id);
    }

    bool HasHandler(uint32_t msg_id) const {
        return message_handler_.HasHandler(msg_id);
    }

    size_t HandlerCount() const {
        return message_handler_.HandlerCount();
    }

private:
    MessageHandlerType message_handler_;
};

}
