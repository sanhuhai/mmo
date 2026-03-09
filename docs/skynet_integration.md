# Skynet 集成指南

本文档介绍如何将 Skynet 集成到 MMO 服务器框架中。

## 概述

Skynet 是一个轻量级的 Actor 模型分布式服务端框架，使用 C + Lua 开发。通过集成 Skynet，MMO 框架可以获得：

- **Actor 模型并发**：基于消息传递的并发模型，避免锁竞争
- **分布式支持**：支持多节点集群部署
- **热更新**：支持 Lua 代码热更新
- **高性能**：基于事件驱动的网络 I/O

## 平台支持

### Windows
- 使用 MinGW 兼容层编译
- 支持 epoll 模拟（wepoll）
- 禁用 jemalloc（使用系统默认内存分配器）

### Linux
- 原生支持 epoll
- 支持 pthread 锁
- 禁用 jemalloc（使用系统默认内存分配器）

## 快速开始

### 1. 创建 Skynet 服务

```cpp
#include "service/skynet_service.h"

// 在 ServiceManager 中创建 Skynet 服务
auto skynet_service = mmo::ServiceManager::Instance().CreateService("skynet", "skynet_node_1");

if (skynet_service) {
    // 启动 Skynet 节点
    auto skynet = std::dynamic_pointer_cast<mmo::SkynetService>(skynet_service);
    if (skynet) {
        skynet->StartNode("config/skynet.config");
    }
}
```

### 2. 配置文件

创建 `config/skynet.config`：

```ini
; 工作线程数
thread = 4

; 模块路径
cpath = "./cservice/?.so"

; Lua 库路径
lpath = "./lualib/?.lua;./third_party/skynet/lualib/?.lua"

; 启动服务
bootstrap = "snlua bootstrap"
start = "main"
```

### 3. Lua 服务示例

创建 `lua/service/my_service.lua`：

```lua
local skynet = require "skynet"

skynet.start(function()
    skynet.error("My service started")
    
    -- 注册消息处理函数
    skynet.dispatch("lua", function(session, address, cmd, ...)
        skynet.error("Received command:", cmd)
        -- 处理命令
    end)
end)
```

## 架构说明

### 组件关系

```
MMO Framework
    │
    ├── ServiceManager
    │       │
    │       └── SkynetService (C++)
    │               │
    │               ├── Skynet Node (C)
    │               │       │
    │               │       ├── Timer
    │               │       ├── Socket
    │               │       └── Worker Threads
    │               │
    │               └── Lua Services
    │                       │
    │                       ├── mmo_bridge.lua
    │                       ├── gate.lua
    │                       └── game_logic.lua
    │
    ├── GateService (C++)
    ├── GameService (C++)
    └── Other Services
```

### 消息流程

1. **MMO → Skynet**：
   ```cpp
   skynet_service->SendMessage(destination, type, data, len);
   ```

2. **Skynet → MMO**：
   ```lua
   -- 在 Lua 中调用 C++ 函数
   mmo.mmo_send_message(service_name, msg_type, data)
   ```

## API 参考

### C++ API

#### SkynetService 类

```cpp
class SkynetService : public ServiceBase {
public:
    // 启动 Skynet 节点
    bool StartNode(const std::string& config_path);
    
    // 发送消息到 Skynet 服务
    bool SendMessage(uint32_t destination, int type, const void* data, size_t len);
    
    // 注册 Lua 服务
    bool RegisterLuaService(const std::string& service_name, const std::string& lua_file);
    
    // 检查节点是否运行
    bool IsNodeRunning() const;
    
    // 获取节点 ID
    uint32_t GetNodeId() const;
};
```

### Lua API

在 Skynet Lua 服务中，可以通过 `mmo` 模块与 MMO 框架交互：

```lua
-- 发送消息到 MMO 服务
mmo.mmo_send_message(service_name, msg_type, data)

-- 注册服务到 MMO ServiceManager
mmo.mmo_register_service(name, handle)

-- 获取 MMO 服务信息
local info = mmo.mmo_get_service_info(name)
```

## 配置选项

### CMake 选项

```cmake
# 启用/禁用 Skynet 集成
option(ENABLE_SKYNET "Enable Skynet integration" ON)

# Windows 特定选项
set(PLATFORM_WINDOWS TRUE)
add_definitions(-DNOUSE_JEMALLOC)

# Linux 特定选项
set(PLATFORM_LINUX TRUE)
add_definitions(-DUSE_PTHREAD_LOCK)
```

### Skynet 配置

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| thread | 工作线程数 | 4 |
| cpath | C 服务模块路径 | "./cservice/?.so" |
| lpath | Lua 库路径 | "./lualib/?.lua" |
| bootstrap | 启动服务 | "snlua bootstrap" |
| start | 主服务 | "main" |
| harbor | 节点 ID（集群模式） | 0 |
| standalone | 独立模式地址 | "0.0.0.0:2013" |

## 注意事项

### Windows 平台

1. **编译要求**：需要 MinGW 或 Visual Studio 2019+
2. **依赖库**：需要链接 ws2_32、winmm
3. **路径格式**：使用正斜杠 `/` 或双反斜杠 `\\`

### Linux 平台

1. **编译要求**：需要 GCC 7+ 或 Clang 6+
2. **依赖库**：需要链接 pthread、dl、m、rt
3. **权限**：可能需要 root 权限绑定低端口

### 通用注意事项

1. **内存管理**：Skynet 使用自己的内存管理机制，注意与 MMO 框架的内存分配器协调
2. **线程安全**：Skynet 的 Actor 模型保证了单线程执行，但跨服务通信需要注意消息顺序
3. **错误处理**：Skynet 的错误处理使用返回值和日志，建议配置适当的日志级别

## 示例代码

### 创建 Skynet 节点

```cpp
void CreateSkynetNode() {
    // 创建 Skynet 服务
    auto service = mmo::ServiceManager::Instance().CreateService(
        "skynet", 
        "node_1"
    );
    
    if (!service) {
        LOG_ERROR("Failed to create skynet service");
        return;
    }
    
    // 获取 Skynet 服务实例
    auto skynet = std::dynamic_pointer_cast<mmo::SkynetService>(service);
    
    // 启动节点
    if (!skynet->StartNode("config/skynet.config")) {
        LOG_ERROR("Failed to start skynet node");
        return;
    }
    
    LOG_INFO("Skynet node started successfully");
}
```

### 从 Lua 调用 MMO 服务

```lua
-- lua/service/game_logic.lua
local skynet = require "skynet"

skynet.start(function()
    -- 注册到 MMO ServiceManager
    mmo.mmo_register_service("game_logic", skynet.self())
    
    -- 处理消息
    skynet.dispatch("lua", function(session, address, cmd, ...)
        if cmd == "player_login" then
            local player_id = ...
            -- 调用 MMO 服务
            local result = mmo.mmo_call_service(
                "player_manager", 
                "load_player", 
                player_id
            )
            skynet.ret(skynet.pack(result))
        end
    end)
end)
```

## 故障排除

### 编译错误

1. **找不到 skynet.h**：
   - 检查 `third_party/skynet` 目录是否存在
   - 重新运行下载脚本

2. **链接错误**：
   - Windows：确保链接了 ws2_32 和 winmm
   - Linux：确保链接了 pthread、dl、m、rt

### 运行时错误

1. **节点启动失败**：
   - 检查配置文件路径是否正确
   - 检查端口是否被占用
   - 查看日志文件获取详细错误信息

2. **消息发送失败**：
   - 检查目标服务地址是否正确
   - 检查消息格式是否正确
   - 确保目标服务已启动

## 参考链接

- [Skynet GitHub](https://github.com/cloudwu/skynet)
- [Skynet Wiki](https://github.com/cloudwu/skynet/wiki)
- [云风的博客](https://blog.codingnow.com/)
