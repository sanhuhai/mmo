#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

#include "cache/redis_connection.h"

namespace mmo {

struct RedisConfig {
    std::string host = "localhost";
    uint16_t port = 6379;
    std::string password;
    uint32_t pool_size = 10;
    uint32_t max_idle_time = 3600;
    bool auto_reconnect = true;
    uint32_t connect_timeout_ms = 5000;
    uint32_t read_timeout_ms = 5000;
};

class RedisConnectionPool {
public:
    using Ptr = std::shared_ptr<RedisConnectionPool>;

    static RedisConnectionPool& Instance() {
        static RedisConnectionPool instance;
        return instance;
    }

    bool Initialize(const RedisConfig& config);
    void Shutdown();

    RedisConnection::Ptr Acquire();
    void Release(RedisConnection::Ptr conn);

    template<typename Func>
    auto Execute(Func&& func) -> decltype(func(std::declval<RedisConnection::Ptr>())) {
        auto conn = Acquire();
        if (!conn) {
            using ReturnType = decltype(func(std::declval<RedisConnection::Ptr>()));
            return ReturnType();
        }
        
        auto result = func(conn);
        Release(conn);
        return result;
    }

    size_t GetPoolSize();
    size_t GetActiveConnections();

private:
    RedisConnectionPool() = default;
    ~RedisConnectionPool() { Shutdown(); }
    RedisConnectionPool(const RedisConnectionPool&) = delete;
    RedisConnectionPool& operator=(const RedisConnectionPool&) = delete;

    RedisConnection::Ptr CreateConnection();
    void Maintenance();

    RedisConfig config_;
    std::queue<RedisConnection::Ptr> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> running_{false};
    std::thread maintenance_thread_;
};

class RedisConnectionGuard {
public:
    explicit RedisConnectionGuard(RedisConnectionPool& pool)
        : pool_(pool), conn_(pool.Acquire()) {
    }

    ~RedisConnectionGuard() {
        if (conn_) {
            pool_.Release(conn_);
        }
    }

    RedisConnection::Ptr operator->() {
        return conn_;
    }

    RedisConnection::Ptr Get() {
        return conn_;
    }

    bool IsValid() const {
        return conn_ != nullptr;
    }

private:
    RedisConnectionPool& pool_;
    RedisConnection::Ptr conn_;
};

}
