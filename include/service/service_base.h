#pragma once

#include "script/lua_engine.h"
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace mmo {

struct ServiceMessage {
    uint32_t source;
    uint32_t destination;
    uint32_t type;
    std::vector<uint8_t> data;
};

class ServiceBase {
public:
    using Ptr = std::shared_ptr<ServiceBase>;
    using MessageHandler = std::function<void(const ServiceMessage&)>;

    ServiceBase(uint32_t id, const std::string& name);
    virtual ~ServiceBase();

    virtual bool Initialize() = 0;
    virtual void Update(int64_t delta_ms) = 0;
    virtual void Shutdown() {}

    void PushMessage(const ServiceMessage& msg);
    bool PopMessage(ServiceMessage& msg);
    size_t GetMessageCount();

    void SetMessageHandler(MessageHandler handler) { message_handler_ = handler; }

    uint32_t GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    bool IsRunning() const { return running_; }
    void SetRunning(bool running) { running_ = running; }

    void Send(uint32_t destination, uint32_t type, const std::vector<uint8_t>& data);
    void Send(uint32_t destination, uint32_t type, const std::string& data);

    LuaEngine& GetLuaEngine() { return lua_engine_; }

protected:
    uint32_t id_;
    std::string name_;
    std::atomic<bool> running_;
    
    std::queue<ServiceMessage> message_queue_;
    std::mutex message_mutex_;
    
    MessageHandler message_handler_;
    LuaEngine lua_engine_;
};

class DefaultService : public ServiceBase {
public:
    DefaultService(uint32_t id, const std::string& name) 
        : ServiceBase(id, name) {}
    
    ~DefaultService() override = default;
    
    bool Initialize() override {
        LOG_INFO("DefaultService '{}' initialized with id {}", GetName(), GetId());
        return true;
    }
    
    void Update(int64_t delta_ms) override {
        ServiceMessage msg;
        while (PopMessage(msg)) {
            if (message_handler_) {
                message_handler_(msg);
            }
        }
    }
    
    void Shutdown() override {
        LOG_INFO("DefaultService '{}' shutdown", GetName());
    }
};

class LuaService : public ServiceBase {
public:
    LuaService(uint32_t id, const std::string& name) 
        : ServiceBase(id, name) {}
    
    ~LuaService() override = default;
    
    bool Initialize() override {
        LOG_INFO("LuaService '{}' initialized with id {}", GetName(), GetId());
        return true;
    }
    
    void Update(int64_t delta_ms) override {
        ServiceMessage msg;
        while (PopMessage(msg)) {
            if (message_handler_) {
                message_handler_(msg);
            }
        }
        
        try {
            auto update_func = luabridge::getGlobal(lua_engine_.GetState(), "on_update");
            if (update_func.isFunction()) {
                update_func(delta_ms);
            }
        } catch (const luabridge::LuaException& e) {
            LOG_ERROR("LuaService '{}' update error: {}", GetName(), e.what());
        }
    }
    
    void Shutdown() override {
        LOG_INFO("LuaService '{}' shutdown", GetName());
    }
};

class ServiceFactory {
public:
    using Creator = std::function<ServiceBase::Ptr(uint32_t, const std::string&)>;

    static ServiceFactory& Instance() {
        static ServiceFactory instance;
        return instance;
    }

    void Register(const std::string& type, Creator creator) {
        creators_[type] = creator;
    }

    ServiceBase::Ptr Create(const std::string& type, uint32_t id, const std::string& name) {
        auto it = creators_.find(type);
        if (it != creators_.end()) {
            return it->second(id, name);
        }
        
        /*if (type == "lua") {
            return std::make_shared<LuaService>(id, name);
        }*/
        
        return std::make_shared<LuaService>(id, name);

        LOG_WARN("Service type '{}' not found, creating DefaultService", type);
        return std::make_shared<DefaultService>(id, name);
    }

    bool HasType(const std::string& type) const {
        return creators_.find(type) != creators_.end();
    }

private:
    ServiceFactory() = default;
    std::unordered_map<std::string, Creator> creators_;
};

template<typename T>
class ServiceRegistrar {
public:
    ServiceRegistrar(const std::string& type) {
        ServiceFactory::Instance().Register(type, [](uint32_t id, const std::string& name) {
            return std::make_shared<T>(id, name);
        });
    }
};

#define REGISTER_SERVICE(type, class_name) \
    static mmo::ServiceRegistrar<class_name> registrar_##class_name(type)

}
