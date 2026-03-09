#pragma once

#include "service/service_base.h"
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

// Skynet headers
extern "C" {
#include "skynet.h"
#include "skynet_handle.h"
#include "skynet_server.h"
#include "skynet_mq.h"
#include "skynet_timer.h"
#include "skynet_socket.h"
}

namespace mmo {

/**
 * @brief Skynet服务适配器类
 * 
 * 将skynet集成到MMO服务框架中，提供Actor模型的并发处理能力
 */
class SkynetService : public ServiceBase {
public:
    SkynetService(uint32_t id, const std::string& name);
    ~SkynetService() override;

    /**
     * @brief 初始化skynet服务
     * @return 初始化是否成功
     */
    bool Initialize() override;

    /**
     * @brief 更新skynet服务
     * @param delta_ms 时间间隔（毫秒）
     */
    void Update(int64_t delta_ms) override;

    /**
     * @brief 关闭skynet服务
     */
    void Shutdown() override;

    /**
     * @brief 启动skynet节点
     * @param config_path 配置文件路径
     * @return 是否启动成功
     */
    bool StartNode(const std::string& config_path);

    /**
     * @brief 发送消息到skynet服务
     * @param destination 目标服务地址
     * @param type 消息类型
     * @param data 消息数据
     * @return 是否发送成功
     */
    bool SendMessage(uint32_t destination, int type, const void* data, size_t len);

    /**
     * @brief 注册Lua服务
     * @param service_name 服务名称
     * @param lua_file Lua文件路径
     * @return 是否注册成功
     */
    bool RegisterLuaService(const std::string& service_name, const std::string& lua_file);

    /**
     * @brief 检查skynet是否正在运行
     * @return 是否正在运行
     */
    bool IsNodeRunning() const { return node_running_; }

    /**
     * @brief 获取skynet节点ID
     * @return 节点ID
     */
    uint32_t GetNodeId() const { return node_id_; }

private:
    /**
     * @brief skynet节点主循环
     */
    void NodeMainLoop();

    /**
     * @brief 处理skynet消息
     */
    void ProcessSkynetMessages();

    /**
     * @brief 初始化skynet环境
     */
    bool InitSkynetEnv();

    /**
     * @brief 清理skynet环境
     */
    void CleanupSkynetEnv();

private:
    std::atomic<bool> node_running_{false};
    std::atomic<bool> initialized_{false};
    
    uint32_t node_id_{0};
    std::string config_path_;
    
    std::thread node_thread_;
    
    // skynet环境配置
    struct skynet_config skynet_config_;
};

/**
 * @brief Skynet服务注册器
 * 
 * 用于自动注册skynet服务到服务工厂
 */
class SkynetServiceRegistrar {
public:
    SkynetServiceRegistrar() {
        ServiceFactory::Instance().Register("skynet", [](uint32_t id, const std::string& name) {
            return std::make_shared<SkynetService>(id, name);
        });
    }
};

// 全局注册器实例
extern SkynetServiceRegistrar g_skynet_service_registrar;

} // namespace mmo
