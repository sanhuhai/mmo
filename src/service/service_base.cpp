#include "service/service_base.h"

namespace mmo {

ServiceBase::ServiceBase(uint32_t id, const std::string& name)
    : id_(id)
    , name_(name)
    , running_(false) {
}

ServiceBase::~ServiceBase() {
    Shutdown();
}

void ServiceBase::PushMessage(const ServiceMessage& msg) {
    std::lock_guard<std::mutex> lock(message_mutex_);
    message_queue_.push(msg);
}

bool ServiceBase::PopMessage(ServiceMessage& msg) {
    std::lock_guard<std::mutex> lock(message_mutex_);
    if (message_queue_.empty()) {
        return false;
    }
    msg = message_queue_.front();
    message_queue_.pop();
    return true;
}

size_t ServiceBase::GetMessageCount() {
    std::lock_guard<std::mutex> lock(message_mutex_);
    return message_queue_.size();
}

void ServiceBase::Send(uint32_t destination, uint32_t type, const std::vector<uint8_t>& data) {
    ServiceMessage msg;
    msg.source = id_;
    msg.destination = destination;
    msg.type = type;
    msg.data = data;
    
    extern void DispatchServiceMessage(const ServiceMessage&);
    DispatchServiceMessage(msg);
}

void ServiceBase::Send(uint32_t destination, uint32_t type, const std::string& data) {
    std::vector<uint8_t> buffer(data.begin(), data.end());
    Send(destination, type, buffer);
}

}
