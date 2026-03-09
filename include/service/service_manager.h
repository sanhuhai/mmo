#pragma once

#include "service/service_base.h"
#include "service/gate_service.h"
#include "service/game_service.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <vector>

namespace mmo {

class ServiceManager {
public:
    static ServiceManager& Instance() {
        static ServiceManager instance;
        return instance;
    }

    bool Initialize() {
        LOG_INFO("Initializing ServiceManager");
        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& pair : services_) {
            pair.second->Shutdown();
        }
        
        services_.clear();
        name_to_id_.clear();
        
        LOG_INFO("ServiceManager shutdown");
    }

    ServiceBase::Ptr CreateService(const std::string& type, const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = name_to_id_.find(name);
        if (it != name_to_id_.end()) {
            LOG_WARN("Service '{}' already exists", name);
            return nullptr;
        }
        
        uint32_t id = next_service_id_++;
        auto service = ServiceFactory::Instance().Create(type, id, name);
        
        if (!service) {
            LOG_ERROR("Failed to create service of type '{}'", type);
            return nullptr;
        }
        
        if (!service->Initialize()) {
            LOG_ERROR("Failed to initialize service '{}'", name);
            return nullptr;
        }
        
        services_[id] = service;
        name_to_id_[name] = id;
        
        LOG_INFO("Service '{}' (type: {}, id: {}) created", name, type, id);
        return service;
    }

    void DestroyService(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = services_.find(id);
        if (it != services_.end()) {
            std::string name = it->second->GetName();
            it->second->Shutdown();
            name_to_id_.erase(name);
            services_.erase(it);
            LOG_INFO("Service '{}' destroyed", name);
        }
    }

    void DestroyService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = name_to_id_.find(name);
        if (it != name_to_id_.end()) {
            DestroyService(it->second);
        }
    }

    ServiceBase::Ptr GetService(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = services_.find(id);
        if (it != services_.end()) {
            return it->second;
        }
        return nullptr;
    }

    ServiceBase::Ptr GetService(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = name_to_id_.find(name);
        if (it != name_to_id_.end()) {
            return GetService(it->second);
        }
        return nullptr;
    }

    void UpdateAll(int64_t delta_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& pair : services_) {
            if (pair.second->IsRunning()) {
                pair.second->Update(delta_ms);
            }
        }
    }

    size_t GetServiceCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return services_.size();
    }

    std::vector<std::string> GetServiceNames() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> names;
        for (const auto& pair : name_to_id_) {
            names.push_back(pair.first);
        }
        return names;
    }

    void DispatchMessage(const ServiceMessage& msg) {
        auto service = GetService(msg.destination);
        if (service) {
            service->PushMessage(msg);
        }
    }

    bool RegisterLuaServices(const std::string& lua_service_dir = "lua/service") {
        LOG_INFO("Registering Lua services from: {}", lua_service_dir);
        
        std::filesystem::path service_path(lua_service_dir);
        if (!std::filesystem::exists(service_path)) {
            LOG_ERROR("Lua service directory not found: {}", lua_service_dir);
            return false;
        }
        
        int registered_count = 0;
        
        for (const auto& entry : std::filesystem::directory_iterator(service_path)) {
            if (entry.is_directory()) {
                std::string service_name = entry.path().filename().string();
                std::string init_file = (entry.path() / "init.lua").string();
                
                if (std::filesystem::exists(init_file)) {
                    auto service = CreateService("lua", service_name);
                    if (service) {
                        std::string module_path = "service." + service_name + ".init";
                        if (service->GetLuaEngine().DoFile(init_file)) {
                            LOG_INFO("Lua service '{}' registered successfully", service_name);
                            registered_count++;
                        } else {
                            LOG_ERROR("Failed to load Lua service module: {}", module_path);
                            DestroyService(service_name);
                        }
                    }
                } else {
                    LOG_WARN("Lua service '{}' missing init.lua, skipping", service_name);
                }
            }
        }
        
        LOG_INFO("Registered {} Lua services", registered_count);
        return registered_count > 0;
    }

private:
    ServiceManager() = default;
    ~ServiceManager() { Shutdown(); }
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    std::unordered_map<uint32_t, ServiceBase::Ptr> services_;
    std::unordered_map<std::string, uint32_t> name_to_id_;
    uint32_t next_service_id_ = 1;
    std::mutex mutex_;
};

inline void DispatchServiceMessage(const ServiceMessage& msg) {
    ServiceManager::Instance().DispatchMessage(msg);
}

}
