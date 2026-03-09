#include "service/skynet_service.h"
#include "core/logger.h"
#include <cstring>
#include <fstream>

namespace mmo {

// 全局注册器实例定义
SkynetServiceRegistrar g_skynet_service_registrar;

SkynetService::SkynetService(uint32_t id, const std::string& name)
    : ServiceBase(id, name) {
    memset(&skynet_config_, 0, sizeof(skynet_config_));
}

SkynetService::~SkynetService() {
    Shutdown();
}

bool SkynetService::Initialize() {
    if (initialized_) {
        LOG_WARN("SkynetService '{}' already initialized", GetName());
        return true;
    }

    LOG_INFO("Initializing SkynetService '{}'", GetName());

    // 初始化skynet环境
    if (!InitSkynetEnv()) {
        LOG_ERROR("Failed to initialize skynet environment for service '{}'", GetName());
        return false;
    }

    initialized_ = true;
    LOG_INFO("SkynetService '{}' initialized successfully", GetName());
    return true;
}

void SkynetService::Update(int64_t delta_ms) {
    if (!initialized_ || !node_running_) {
        return;
    }

    // 处理skynet消息
    ProcessSkynetMessages();

    // 处理MMO框架消息
    ServiceMessage msg;
    while (PopMessage(msg)) {
        if (message_handler_) {
            message_handler_(msg);
        }
        
        // 将MMO消息转发到skynet
        // TODO: 实现消息转换和转发逻辑
    }
}

void SkynetService::Shutdown() {
    if (!initialized_) {
        return;
    }

    LOG_INFO("Shutting down SkynetService '{}'", GetName());

    // 停止skynet节点
    node_running_ = false;
    
    if (node_thread_.joinable()) {
        node_thread_.join();
    }

    // 清理skynet环境
    CleanupSkynetEnv();

    initialized_ = false;
    LOG_INFO("SkynetService '{}' shutdown complete", GetName());
}

bool SkynetService::StartNode(const std::string& config_path) {
    if (!initialized_) {
        LOG_ERROR("SkynetService '{}' not initialized", GetName());
        return false;
    }

    if (node_running_) {
        LOG_WARN("Skynet node already running for service '{}'", GetName());
        return true;
    }

    config_path_ = config_path;

    // 检查配置文件是否存在
    std::ifstream config_file(config_path);
    if (!config_file.good()) {
        LOG_ERROR("Skynet config file not found: {}", config_path);
        return false;
    }
    config_file.close();

    LOG_INFO("Starting skynet node with config: {}", config_path);

    // 在独立线程中启动skynet节点
    node_thread_ = std::thread(&SkynetService::NodeMainLoop, this);
    
    // 等待节点启动
    int retry_count = 0;
    while (!node_running_ && retry_count < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        retry_count++;
    }

    if (!node_running_) {
        LOG_ERROR("Skynet node failed to start for service '{}'", GetName());
        return false;
    }

    LOG_INFO("Skynet node started successfully for service '{}'", GetName());
    return true;
}

bool SkynetService::SendMessage(uint32_t destination, int type, const void* data, size_t len) {
    if (!node_running_) {
        LOG_ERROR("Cannot send message, skynet node not running");
        return false;
    }

    // 使用skynet API发送消息
    struct skynet_message msg;
    msg.source = GetNodeId();
    msg.session = 0;
    msg.data = skynet_malloc(len);
    if (!msg.data) {
        LOG_ERROR("Failed to allocate memory for skynet message");
        return false;
    }
    
    memcpy(msg.data, data, len);
    msg.sz = len | ((size_t)type << MESSAGE_TYPE_SHIFT);

    if (skynet_context_push(destination, &msg)) {
        LOG_ERROR("Failed to push message to skynet context {}", destination);
        skynet_free(msg.data);
        return false;
    }

    return true;
}

bool SkynetService::RegisterLuaService(const std::string& service_name, const std::string& lua_file) {
    if (!node_running_) {
        LOG_ERROR("Cannot register Lua service, skynet node not running");
        return false;
    }

    LOG_INFO("Registering Lua service '{}' with file '{}'", service_name, lua_file);

    // 创建snlua服务来加载Lua文件
    // TODO: 实现Lua服务注册逻辑

    return true;
}

void SkynetService::NodeMainLoop() {
    LOG_INFO("Skynet node main loop started for service '{}'", GetName());

    // 启动skynet节点
    // 注意：这里需要调用skynet的启动函数
    // 由于skynet的启动是阻塞的，我们需要在单独的线程中运行

    node_running_ = true;

    // 模拟skynet主循环
    while (node_running_) {
        // 处理skynet定时器
        skynet_updatetime();
        
        // 处理socket事件
        skynet_socket_poll();
        
        // 休眠一小段时间避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LOG_INFO("Skynet node main loop ended for service '{}'", GetName());
}

void SkynetService::ProcessSkynetMessages() {
    // 处理skynet消息队列中的消息
    // TODO: 实现消息处理逻辑
}

bool SkynetService::InitSkynetEnv() {
    LOG_INFO("Initializing skynet environment");

    // 初始化skynet全局环境
    // 设置默认配置
    skynet_config_.thread = 4;  // 默认工作线程数
    skynet_config_.cpath = "./cservice/?.so";
    skynet_config_.lpath = "./lualib/?.lua;./lualib/?/init.lua";
    skynet_config_.logger = nullptr;
    skynet_config_.logservice = "logger";
    skynet_config_.profile = 1;

    // 初始化skynet全局模块
    skynet_globalinit();
    
    // 初始化skynet环境
    skynet_env_init();

    LOG_INFO("Skynet environment initialized");
    return true;
}

void SkynetService::CleanupSkynetEnv() {
    LOG_INFO("Cleaning up skynet environment");

    // 清理skynet全局模块
    skynet_globalexit();

    LOG_INFO("Skynet environment cleaned up");
}

} // namespace mmo
