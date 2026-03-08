# MMO Server - 高性能游戏服务器框架

基于C++的高性能游戏服务器框架，使用Lua作为脚本语言，LuaBridge实现C++到Lua的绑定，采用Skynet风格的服务架构。

## 特性

- **高性能网络**: 基于Asio实现异步TCP网络通信
- **Lua脚本支持**: 使用LuaBridge绑定C++函数到Lua
- **Skynet风格服务**: 服务隔离、消息传递的微服务架构
- **灵活配置**: INI格式配置文件
- **日志系统**: 支持spdlog高性能日志
- **跨平台**: 支持Windows/Linux/macOS

## 目录结构

```
mmo/
├── include/              # 头文件
│   ├── core/            # 核心模块
│   ├── network/         # 网络模块
│   ├── script/          # 脚本引擎
│   ├── service/         # 服务模块
│   └── utils/           # 工具类
├── src/                  # 源代码
│   ├── core/
│   ├── network/
│   ├── script/
│   ├── service/
│   ├── utils/
│   └── main.cpp
├── lua/                  # Lua脚本
│   ├── main.lua         # 主入口脚本
│   ├── service/         # 服务脚本
│   └── example/         # 示例脚本
├── config/               # 配置文件
├── third_party/          # 第三方库
├── build/                # 构建目录
├── CMakeLists.txt        # CMake配置
├── build.bat             # Windows构建脚本
└── build.sh              # Linux构建脚本
```

## 依赖库

| 库名 | 版本 | 说明 |
|------|------|------|
| Lua | 5.4+ | 脚本语言 |
| LuaBridge | 2.8+ | C++/Lua绑定 |
| Asio | 1.28+ | 异步网络库 |
| spdlog | 1.12+ | 日志库(可选) |

## 编译

### Windows (Visual Studio)

```powershell
# 安装依赖 (使用vcpkg)
vcpkg install lua:x64-windows spdlog:x64-windows

# 下载LuaBridge和Asio到third_party目录
cd third_party
git clone https://github.com/vinniefalco/LuaBridge.git
git clone https://github.com/chriskohlhoff/asio.git

# 构建
.\build.bat
```

### Linux

```bash
# 安装依赖
sudo apt-get install lua5.4 liblua5.4-dev libspdlog-dev cmake g++

# 下载LuaBridge和Asio
cd third_party
git clone https://github.com/vinniefalco/LuaBridge.git
git clone https://github.com/chriskohlhoff/asio.git

# 构建
chmod +x build.sh
./build.sh
```

## 运行

```bash
# Windows
build\bin\Release\mmo_server.exe -c config\server.ini

# Linux
./build/bin/mmo_server -c config/server.ini
```

## 配置文件

配置文件格式为INI，示例 (`config/server.ini`):

```ini
server.name = MMO-Server
server.id = 1

[log]
file = logs/server.log
level = 2

[gate]
port = 8888
max_connections = 1000

[game]
max_players = 5000
```

## Lua脚本开发

### C++函数注册到Lua

在 `script/lua_register.h` 中注册C++函数：

```cpp
luabridge::getGlobalNamespace(engine.GetState())
    .beginNamespace("mmo")
        .addFunction("log_info", [](const std::string& msg) {
            LOG_INFO("{}", msg);
        })
    .endNamespace();
```

### Lua中调用C++函数

```lua
-- 日志
mmo.log_info("Hello from Lua!")
mmo.log_debug("Debug message")
mmo.log_error("Error message")

-- 工具函数
local time_ms = mmo.utils.time_ms()
local time_sec = mmo.utils.time_sec()

-- 配置
local server_name = config:get("server.name", "Default"):as_string()
```

### 服务脚本

每个服务都有自己的Lua脚本目录：

```lua
-- lua/service/gate1/init.lua

function on_client_connect(conn_id, address)
    mmo.log_info("Client " .. conn_id .. " connected from " .. address)
    gate.send_to_client(conn_id, "Welcome!")
end

function on_client_message(conn_id, message)
    mmo.log_debug("Message from " .. conn_id .. ": " .. message)
    gate.send_to_client(conn_id, "Echo: " .. message)
end

function on_client_disconnect(conn_id)
    mmo.log_info("Client " .. conn_id .. " disconnected")
end
```

## 架构设计

### 服务模型 (Skynet风格)

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ GateService │────▶│ GameService │────▶│  DBService  │
│  (网络网关)  │     │  (游戏逻辑)  │     │  (数据存储)  │
└─────────────┘     └─────────────┘     └─────────────┘
       │                   │                   │
       └───────────────────┴───────────────────┘
                    消息队列通信
```

### 消息传递

```cpp
// 服务间消息
ServiceMessage msg;
msg.source = gate_service_id;
msg.destination = game_service_id;
msg.type = MSG_PLAYER_LOGIN;
msg.data = player_data;

DispatchServiceMessage(msg);
```

## 扩展开发

### 创建新服务

1. 继承 `ServiceBase` 类：

```cpp
class MyService : public ServiceBase {
public:
    MyService(uint32_t id, const std::string& name)
        : ServiceBase(id, name) {}
    
    bool Initialize() override;
    void Update(int64_t delta_ms) override;
    void Shutdown() override;
};

REGISTER_SERVICE("my_service", MyService)
```

2. 创建Lua脚本：

```lua
-- lua/service/my_service/init.lua

function on_update(delta_ms)
    -- 服务更新逻辑
end

function on_service_message(source, type, data)
    -- 处理来自其他服务的消息
end
```

## 协议格式

网络消息格式（大端序）：

```
┌────────────┬─────────────────┐
│ Size(4B)   │     Data        │
│ 消息长度    │    消息内容      │
└────────────┴─────────────────┘
```

## 性能优化

- 使用对象池减少内存分配
- 异步I/O避免阻塞
- 服务间无锁消息队列
- Lua脚本热重载

## License

MIT License
