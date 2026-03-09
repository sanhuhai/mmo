#include "network/network_processor.h"
#include "core/logger.h"
#include <chrono>

namespace mmo {

NetworkMessageProcessor::NetworkMessageProcessor()
    : message_queue_(10000)
    , running_(false) {
}

NetworkMessageProcessor::~NetworkMessageProcessor() {
    Shutdown();
}

bool NetworkMessageProcessor::Initialize() {
    LOG_INFO("Network message processor initialized");
    return true;
}

void NetworkMessageProcessor::Shutdown() {
    Stop();
    LOG_INFO("Network message processor shutdown");
}

bool NetworkMessageProcessor::Start() {
    if (running_) {
        LOG_WARN("Network message processor already running");
        return false;
    }

    running_ = true;

    message_queue_.SetHandler([this](const NetworkPacket& packet) {
        ProcessMessage(packet);
    });

    message_queue_.Start();

    LOG_INFO("Network message processor started");
    return true;
}

void NetworkMessageProcessor::Stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    message_queue_.Stop();

    LOG_INFO("Network message processor stopped");
}

bool NetworkMessageProcessor::ProcessPacket(const NetworkPacket& packet) {
    if (!packet.IsValid()) {
        LOG_ERROR("Invalid packet: msg_id={}, data_size={}", packet.msg_id, packet.data.size());
        return false;
    }

    return message_queue_.Push(packet);
}

void NetworkMessageProcessor::ProcessMessage(const NetworkPacket& packet) {
    try {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto it = handlers_.find(packet.msg_id);
        if (it != handlers_.end()) {
            it->second->Handle(packet.data, packet.session_id);
        } else {
            LOG_WARN("No handler registered for msg_id {}", packet.msg_id);
        }
    } catch (const std::exception& e) {
        HandleError(e.what());
    }
}

void NetworkMessageProcessor::HandleError(const std::string& error) {
    LOG_ERROR("Network message processor error: {}", error);
}

void NetworkMessageProcessor::UnregisterHandler(uint32_t msg_id) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.erase(msg_id);
}

size_t NetworkMessageProcessor::GetQueueSize() const {
    return message_queue_.Size();
}

bool NetworkMessageProcessor::IsQueueFull() const {
    return message_queue_.IsFull();
}

}
