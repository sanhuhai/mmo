# MMO Server - 高性能游戏服务器框架

基于C++17的高性能游戏服务器框架，使用Lua作为脚本语言，LuaBridge3实现C++到Lua的绑定，采用Skynet风格的服务架构，支持Protobuf协议通信和Redis缓存。

## 特性

- **高性能网络**: 基于Asio实现异步TCP网络通信，支持Protobuf协议
- **Lua脚本支持**: 使用LuaBridge3绑定C++函数到Lua，支持模块化调用
- **Skynet风格服务**: 服务隔离、消息传递的微服务架构
- **Redis缓存系统**: 双层缓存架构，支持定时同步到数据库
- **MySQL数据库**: 连接池管理，支持玩家数据持久化
- **Protobuf协议**: 高效的二进制序列化，支持Lua端使用
- **灵活配置**: INI格式配置文件
- **日志系统**: 支持spdlog高性能日志
- **跨平台**: 支持Windows/Linux/macOS
- **调试支持**: 支持LuaPanda调试器

## 目录结构

```
mmo/
├── include/              # 头文件
│   ├── cache/           # 缓存模块 (Redis缓存管理)
│   ├── core/            # 核心模块 (应用、配置、日志)
│   ├── db/              # 数据库模块 (MySQL连接池)
│   ├── network/         # 网络模块 (TCP服务器、消息处理)
│   ├── player/          # 玩家模块 (玩家管理、武器系统)
│   ├── proto/           # 协议模块 (Protobuf编解码)
│   ├── script/          # 脚本引擎 (Lua引擎、注册绑定)
│   ├── service/         # 服务模块 (服务基类、网关、游戏服务)
│   └── utils/           # 工具类 (线程池、定时器、缓冲区)
├── src/                  # 源代码
│   ├── cache/
│   ├── core/
│   ├── db/
│   ├── network/
│   ├── player/
│   ├── proto/
│   ├── script/
│   ├── service/
│   ├── utils/
│   └── main.cpp
├── lua/                  # Lua脚本
│   ├── main.lua         # 主入口脚本
│   ├── service/         # 服务脚本
│   │   ├── gate1/      # 网关服务脚本
│   │   └── game1/      # 游戏服务脚本
│   ├── example/         # 示例脚本
│   └── LuaPanda.lua     # 调试器支持
├── proto/                # Protobuf协议定义
│   ├── login.proto      # 登录协议
│   ├── player.proto     # 玩家协议
│   ├── game.proto       # 游戏协议
│   ├── chat.proto       # 聊天协议
│   ├── network.proto    # 网络协议
│   └── common.proto     # 公共协议
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
| LuaBridge | 3.0+ | C++/Lua绑定 |
| Asio | 1.28+ | 异步网络库 (header-only) |
| Protobuf | 3.21+ | 序列化协议 |
| hiredis | 1.2+ | Redis客户端 |
| MySQL | 8.0+ | 数据库连接 |
| spdlog | 1.12+ | 日志库 (可选) |

## 编译

### Windows (Visual Studio)

```powershell
# 配置项目
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release

# 构建特定目标
cmake --build build --config Release --target mmo_server
cmake --build build --config Release --target test_network_client
```

### Linux

```bash
# 安装依赖
sudo apt-get install lua5.4 liblua5.4-dev libspdlog-dev cmake g++

# 配置项目
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release
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
[server]
name = MMO-Server
id = 1

[log]
file = logs/server.log
level = 2

[network]
port = 8888

[gate]
port = 8888
max_connections = 1000

[game]
max_players = 5000

[mysql]
host = localhost
port = 3306
user = root
password = your_password
database = mmo_server
pool_size = 10

[redis]
host = localhost
port = 6379
password = 
pool_size = 10
```

## 网络协议

### 消息格式

网络消息格式（小端序）：

```
┌────────────┬────────────┬─────────────────┐
│ MsgID(4B)  │ MsgLen(4B) │     Data        │
│ 消息ID     │ 消息长度    │    消息内容      │
└────────────┴────────────┴─────────────────┘
```

### 消息ID定义

| 消息ID | 消息类型 | 说明 |
|--------|----------|------|
| 1 | LoginRequest/LoginResponse | 登录请求/响应 |
| 2 | LogoutRequest/LogoutResponse | 登出请求/响应 |
| 100 | Heartbeat | 心跳消息 |

### Protobuf协议示例

```protobuf
// proto/login.proto
syntax = "proto3";
package mmo;

message LoginRequest {
    string account = 1;
    string password = 2;
    string platform = 3;
    string version = 4;
    string device_id = 5;
}

message LoginResponse {
    Result result = 1;
    uint64 player_id = 2;
    string token = 3;
    string server_time = 4;
}
```

## 缓存系统

### 架构设计

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  LocalCache │────▶│    Redis    │────▶│    MySQL    │
│  (本地缓存)  │     │  (分布式缓存) │     │  (持久存储)  │
└─────────────┘     └─────────────┘     └─────────────┘
       │                   │                   │
       └───────────────────┴───────────────────┘
              每3分钟自动同步脏数据
```

### 使用示例

```cpp
// 初始化缓存
mmo::RedisConfig redis_config;
redis_config.host = "localhost";
redis_config.port = 6379;

auto redis_pool = std::make_shared<mmo::RedisConnectionPool>();
redis_pool->Initialize(redis_config);

mmo::CacheConfig cache_config;
cache_config.sync_interval = std::chrono::seconds(180);  // 3分钟同步
cache_config.auto_sync = true;

auto& cache = mmo::CacheManager::Instance();
cache.Initialize(redis_pool, cache_config);

// 使用缓存
cache.Set("player:1001", R"({"name":"张三","level":50})", "players", "1001");
std::string data = cache.Get("player:1001");

// Hash操作
cache.HSet("player:1001", "name", "张三", "players", "1001");
cache.HSet("player:1001", "level", "50");
auto all_data = cache.HGetAll("player:1001");

// 获取统计信息
auto stats = cache.GetStats();
cache.PrintStats();
```

### 缓存特性

| 功能 | 说明 |
|------|------|
| 双层缓存 | 本地缓存 + Redis缓存 |
| 自动同步 | 定时同步脏数据到数据库 |
| LRU淘汰 | 自动清理最少使用的缓存 |
| TTL支持 | 支持设置过期时间 |
| 脏标记 | 跟踪修改，只同步变更数据 |

## Lua脚本开发

### 模块化调用

C++端调用Lua模块：

```cpp
mmo::LuaEngine lua_engine;
lua_engine.Initialize();
lua_engine.SetScriptPath("../lua");

// 加载模块
lua_engine.LoadModule("player_manager");
lua_engine.LoadAllMoudle();

// 调用模块方法（无返回值）
lua_engine.CallModule("weapon_manager", "test_weapon_types");

// 调用模块方法（带返回值）
std::string greeting = lua_engine.CallModuleWithReturn<std::string>(
    "test_module", "hello", "World");
int sum = lua_engine.CallModuleWithReturn<int>(
    "test_module", "add", 10, 20);
```

### Lua模块示例

```lua
-- lua/player_manager.lua
local PlayerManager = {}

function PlayerManager.load_all_players()
    print("Loading all players...")
    return true
end

function PlayerManager.get_player(player_id)
    -- 从缓存获取玩家数据
    local data = cache.get("player:" .. player_id)
    return data
end

function PlayerManager.save_player(player_id, data)
    -- 保存到缓存，标记为脏数据
    cache.set("player:" .. player_id, data, "players", player_id)
    return true
end

return PlayerManager
```

### C++函数注册到Lua

```cpp
// 在 script/lua_register.h 中注册
luabridge::getGlobalNamespace(engine.GetState())
    .beginNamespace("mmo")
        .addFunction("log_info", [](const std::string& msg) {
            LOG_INFO("{}", msg);
        })
        .addFunction("log_debug", [](const std::string& msg) {
            LOG_DEBUG("{}", msg);
        })
        .addFunction("log_error", [](const std::string& msg) {
            LOG_ERROR("{}", msg);
        })
    .endNamespace();
```

### Lua中调用C++函数

```lua
-- 日志
mmo.log_info("Hello from Lua!")
mmo.log_debug("Debug message")
mmo.log_error("Error message")

-- 缓存操作
cache.set("key", "value", "table_name", "primary_key")
local value = cache.get("key")
cache.hset("player:1001", "name", "张三")
local name = cache.hget("player:1001", "name")

-- 配置
local server_name = config:get("server.name", "Default"):as_string()
```

## 服务架构

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

### 创建新服务

```cpp
class MyService : public ServiceBase {
public:
    MyService(uint32_t id, const std::string& name)
        : ServiceBase(id, name) {}
    
    bool Initialize() override {
        LOG_INFO("MyService initialized");
        return true;
    }
    
    void Update(int64_t delta_ms) override {
        // 服务更新逻辑
    }
    
    void Shutdown() override {
        LOG_INFO("MyService shutdown");
    }
};

REGISTER_SERVICE("my_service", MyService)
```

### 服务脚本

```lua
-- lua/service/gate1/init.lua

function on_client_connect(conn_id, address)
    mmo.log_info("Client " .. conn_id .. " connected from " .. address)
end

function on_client_message(conn_id, message)
    mmo.log_debug("Message from " .. conn_id .. ": " .. message)
end

function on_client_disconnect(conn_id)
    mmo.log_info("Client " .. conn_id .. " disconnected")
end
```

## 调试支持

### LuaPanda调试配置

项目已配置LuaPanda调试支持，使用attach模式。

`.vscode/launch.json`:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lua",
            "request": "attach",
            "name": "LuaPanda Attach",
            "luaPanda": {
                "luaPath": "lua",
                "connectionPort": 8818
            }
        }
    ]
}
```

### Debug模式构建

```powershell
# 配置Debug构建
cmake -B build_debug -S . -DCMAKE_BUILD_TYPE=Debug

# 构建Debug版本
cmake --build build_debug --config Debug
```

## 测试客户端

项目包含测试客户端用于验证网络通信：

```powershell
# 启动服务器
build\bin\Release\mmo_server.exe

# 运行测试客户端
build\bin\Release\test_network_client.exe
```

测试客户端功能：
- 发送Protobuf格式的登录请求
- 接收并解析服务器响应
- 发送心跳消息

## 性能优化

- 使用对象池减少内存分配
- 异步I/O避免阻塞
- 服务间无锁消息队列
- 双层缓存减少数据库访问
- Lua脚本热重载
- Protobuf高效序列化

## API参考

### CacheManager API

| 方法 | 说明 |
|------|------|
| `Set(key, value, table, pk, ttl)` | 设置缓存 |
| `Get(key)` | 获取缓存 |
| `Del(key)` | 删除缓存 |
| `HSet(key, field, value, table, pk)` | 设置Hash字段 |
| `HGet(key, field)` | 获取Hash字段 |
| `HGetAll(key)` | 获取所有Hash字段 |
| `MarkDirty(key)` | 标记为脏数据 |
| `SyncToDB(key)` | 同步到数据库 |
| `SyncAllToDB()` | 同步所有脏数据 |
| `GetStats()` | 获取统计信息 |

### LuaEngine API

| 方法 | 说明 |
|------|------|
| `Initialize()` | 初始化引擎 |
| `SetScriptPath(path)` | 设置脚本路径 |
| `LoadModule(name)` | 加载模块 |
| `LoadAllMoudle()` | 加载所有模块 |
| `CallModule(module, method, ...)` | 调用模块方法 |
| `CallModuleWithReturn<T>(module, method, ...)` | 调用并返回值 |
| `DoFile(path)` | 执行脚本文件 |
| `Shutdown()` | 关闭引擎 |

## License

MIT License
