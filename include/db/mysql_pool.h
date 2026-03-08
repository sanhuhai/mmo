#pragma once

#ifdef USE_MYSQL

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

#include "db/mysql_connection.h"

namespace mmo {

struct MySQLConfig {
    std::string host = "localhost";
    uint16_t port = 3306;
    std::string user;
    std::string password;
    std::string database;
    uint32_t pool_size = 10;
    uint32_t max_idle_time = 3600;
    bool auto_reconnect = true;
};

class MySQLConnectionPool {
public:
    using Ptr = std::shared_ptr<MySQLConnectionPool>;

    static MySQLConnectionPool& Instance() {
        static MySQLConnectionPool instance;
        return instance;
    }

    bool Initialize(const MySQLConfig& config) {
        config_ = config;
        
        for (uint32_t i = 0; i < config_.pool_size; ++i) {
            auto conn = CreateConnection();
            if (conn) {
                std::lock_guard<std::mutex> lock(mutex_);
                pool_.push(conn);
            }
        }

        if (pool_.empty()) {
            LOG_ERROR("Failed to create any MySQL connections");
            return false;
        }

        running_ = true;
        maintenance_thread_ = std::thread(&MySQLConnectionPool::Maintenance, this);
        
        LOG_INFO("MySQL connection pool initialized with {} connections", pool_.size());
        return true;
    }

    void Shutdown() {
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

        LOG_INFO("MySQL connection pool shutdown");
    }

    MySQLConnection::Ptr Acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto timeout = std::chrono::seconds(5);
        bool acquired = cond_.wait_for(lock, timeout, [this]() {
            return !pool_.empty() || !running_;
        });

        if (!acquired || pool_.empty()) {
            LOG_WARN("Failed to acquire MySQL connection within timeout");
            return CreateConnection();
        }

        auto conn = pool_.front();
        pool_.pop();
        
        if (config_.auto_reconnect && !conn->Ping()) {
            LOG_INFO("Reconnecting MySQL connection...");
            if (!conn->Reconnect()) {
                LOG_ERROR("Failed to reconnect MySQL connection");
                pool_.push(conn);
                return nullptr;
            }
        }

        return conn;
    }

    void Release(MySQLConnection::Ptr conn) {
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

    template<typename Func>
    auto Execute(Func&& func) -> decltype(func(std::declval<MySQLConnection::Ptr>())) {
        auto conn = Acquire();
        if (!conn) {
            using ReturnType = decltype(func(std::declval<MySQLConnection::Ptr>()));
            return ReturnType();
        }
        
        auto result = func(conn);
        Release(conn);
        return result;
    }

    size_t GetPoolSize() {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    size_t GetActiveConnections() {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.pool_size - pool_.size();
    }

private:
    MySQLConnectionPool() = default;
    ~MySQLConnectionPool() { Shutdown(); }
    MySQLConnectionPool(const MySQLConnectionPool&) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool&) = delete;

    MySQLConnection::Ptr CreateConnection() {
        auto conn = std::make_shared<MySQLConnection>();
        if (conn->Connect(config_.host, config_.port, config_.user, 
                          config_.password, config_.database)) {
            return conn;
        }
        return nullptr;
    }

    void Maintenance() {
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
                LOG_INFO("MySQL pool maintenance: created {} connections", 
                         pool_.size() - current_size);
            }
        }
    }

    MySQLConfig config_;
    std::queue<MySQLConnection::Ptr> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> running_{false};
    std::thread maintenance_thread_;
};

class MySQLConnectionGuard {
public:
    explicit MySQLConnectionGuard(MySQLConnectionPool& pool)
        : pool_(pool), conn_(pool.Acquire()) {
    }

    ~MySQLConnectionGuard() {
        if (conn_) {
            pool_.Release(conn_);
        }
    }

    MySQLConnection::Ptr operator->() {
        return conn_;
    }

    MySQLConnection::Ptr Get() {
        return conn_;
    }

    bool IsValid() const {
        return conn_ != nullptr;
    }

private:
    MySQLConnectionPool& pool_;
    MySQLConnection::Ptr conn_;
};

}

#endif
