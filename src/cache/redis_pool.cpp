#include "cache/redis_pool.h"

namespace mmo {

bool RedisConnectionPool::Initialize(const RedisConfig& config) {
    config_ = config;
    
    for (uint32_t i = 0; i < config_.pool_size; ++i) {
        auto conn = CreateConnection();
        if (conn) {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_.push(conn);
        }
    }

    if (pool_.empty()) {
        LOG_ERROR("Failed to create any Redis connections");
        return false;
    }

    running_ = true;
    maintenance_thread_ = std::thread(&RedisConnectionPool::Maintenance, this);
    
    LOG_INFO("Redis connection pool initialized with {} connections", pool_.size());
    return true;
}

void RedisConnectionPool::Shutdown() {
    running_ = false;
    cond_.notify_all();
    
    if (maintenance_thread_.joinable()) {
        maintenance_thread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.front()->Disconnect();
        pool_.pop();
    }

    LOG_INFO("Redis connection pool shutdown");
}

RedisConnection::Ptr RedisConnectionPool::Acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto timeout = std::chrono::seconds(5);
    bool acquired = cond_.wait_for(lock, timeout, [this]() {
        return !pool_.empty() || !running_;
    });

    if (!acquired || pool_.empty()) {
        LOG_WARN("Failed to acquire Redis connection within timeout");
        return CreateConnection();
    }

    auto conn = pool_.front();
    pool_.pop();
    
    if (config_.auto_reconnect && !conn->Ping()) {
        LOG_INFO("Reconnecting Redis connection...");
        if (!conn->Reconnect()) {
            LOG_ERROR("Failed to reconnect Redis connection");
            pool_.push(conn);
            return nullptr;
        }
    }

    return conn;
}

void RedisConnectionPool::Release(RedisConnection::Ptr conn) {
    if (!conn) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        pool_.push(conn);
        cond_.notify_one();
    } else {
        conn->Disconnect();
    }
}

size_t RedisConnectionPool::GetPoolSize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.size();
}

size_t RedisConnectionPool::GetActiveConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.pool_size - pool_.size();
}

RedisConnection::Ptr RedisConnectionPool::CreateConnection() {
    auto conn = std::make_shared<RedisConnection>();
    if (conn->Connect(config_.host, config_.port, config_.password)) {
        return conn;
    }
    return nullptr;
}

void RedisConnectionPool::Maintenance() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        
        if (!running_) break;

        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t current_size = pool_.size();
        size_t target_size = config_.pool_size;
        
        if (current_size < target_size) {
            size_t to_create = target_size - current_size;
            for (size_t i = 0; i < to_create; ++i) {
                auto conn = CreateConnection();
                if (conn) {
                    pool_.push(conn);
                }
            }
            LOG_INFO("Redis pool maintenance: created {} connections", 
                     pool_.size() - current_size);
        }
    }
}

}
