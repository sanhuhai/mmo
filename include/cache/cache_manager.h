#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

#include "cache/redis_pool.h"
#include "core/logger.h"

#ifdef USE_MYSQL
#include "db/mysql_pool.h"
#endif

namespace mmo {

struct CacheEntry {
    std::string key;
    std::string value;
    std::string table_name;
    std::string primary_key;
    std::chrono::steady_clock::time_point last_access;
    std::chrono::steady_clock::time_point last_modify;
    std::chrono::steady_clock::time_point created_at;
    bool dirty = false;
    int access_count = 0;
    int64_t ttl_seconds = 0;
};

struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t writes = 0;
    uint64_t reads = 0;
    uint64_t syncs = 0;
    uint64_t sync_errors = 0;
    uint64_t evictions = 0;
    size_t cache_size = 0;
    size_t dirty_entries = 0;
    double hit_rate = 0.0;
};

struct CacheConfig {
    std::chrono::seconds sync_interval{180};
    bool auto_sync = true;
    bool sync_on_shutdown = true;
    size_t max_cache_size = 100000;
    std::chrono::seconds default_ttl{3600};
    bool enable_local_cache = true;
};

class CacheManager {
public:
    using Ptr = std::shared_ptr<CacheManager>;
    using SyncCallback = std::function<bool(const std::string& table, const std::string& key, const std::string& value)>;
    using SerializeCallback = std::function<std::string(const std::string& key)>;
    using DeserializeCallback = std::function<void(const std::string& key, const std::string& value)>;

    static CacheManager& Instance() {
        static CacheManager instance;
        return instance;
    }

    bool Initialize(RedisConnectionPool::Ptr redis_pool, const CacheConfig& config = CacheConfig{});
    
#ifdef USE_MYSQL
    bool InitializeWithMySQL(RedisConnectionPool::Ptr redis_pool, 
                             std::shared_ptr<MySQLConnectionPool> mysql_pool,
                             const CacheConfig& config = CacheConfig{});
#endif

    void Shutdown();

    void SetConfig(const CacheConfig& config);
    CacheConfig GetConfig() const { return config_; }

    void SetSyncCallback(SyncCallback callback) { sync_callback_ = callback; }
    void SetSerializeCallback(SerializeCallback callback) { serialize_callback_ = callback; }
    void SetDeserializeCallback(DeserializeCallback callback) { deserialize_callback_ = callback; }

    void EnableAutoSync(bool enable);
    bool IsAutoSyncEnabled() const { return config_.auto_sync; }

    void SetSyncInterval(std::chrono::seconds interval);
    std::chrono::seconds GetSyncInterval() const { return config_.sync_interval; }

    bool Set(const std::string& key, const std::string& value, 
             const std::string& table = "", const std::string& primary_key = "",
             int64_t ttl_seconds = 0);
    
    std::string Get(const std::string& key);
    
    bool Del(const std::string& key);
    
    bool Exists(const std::string& key);

    bool SetEx(const std::string& key, const std::string& value, int64_t ttl_seconds);
    
    bool Expire(const std::string& key, int64_t ttl_seconds);
    
    int64_t TTL(const std::string& key);

    bool HSet(const std::string& key, const std::string& field, const std::string& value,
              const std::string& table = "", const std::string& primary_key = "");
    
    std::string HGet(const std::string& key, const std::string& field);
    
    std::map<std::string, std::string> HGetAll(const std::string& key);
    
    bool HDel(const std::string& key, const std::string& field);
    
    bool HExists(const std::string& key, const std::string& field);

    void MarkDirty(const std::string& key);
    void MarkClean(const std::string& key);
    bool IsDirty(const std::string& key);

    bool SyncToDB(const std::string& key);
    bool SyncAllToDB();
    int SyncDirtyEntries();
    size_t GetDirtyCount() const;

#ifdef USE_MYSQL
    bool LoadFromMySQL(const std::string& table, const std::string& primary_key);
    bool LoadQueryToCache(const std::string& sql, const std::string& table, 
                          const std::string& key_column = "id");
#endif

    void WarmCache(const std::string& table, const std::string& where_clause = "");

    CacheStats GetStats() const;
    void ResetStats();
    void PrintStats() const;

    void Clear();
    void ClearLocal();
    void ClearRedis();
    
    void EvictExpired();
    void EvictLRU(size_t count);
    void EvictByPattern(const std::string& pattern);

    template<typename T>
    bool SetObject(const std::string& key, const T& obj,
                   const std::string& table = "", const std::string& primary_key = "",
                   int64_t ttl_seconds = 0) {
        std::string serialized = Serialize(obj);
        return Set(key, serialized, table, primary_key, ttl_seconds);
    }

    template<typename T>
    bool GetObject(const std::string& key, T& obj) {
        std::string value = Get(key);
        if (value.empty()) {
            return false;
        }
        return Deserialize(value, obj);
    }

    template<typename T>
    bool HSetObject(const std::string& key, const std::string& field, const T& obj,
                    const std::string& table = "", const std::string& primary_key = "") {
        std::string serialized = Serialize(obj);
        return HSet(key, field, serialized, table, primary_key);
    }

    template<typename T>
    bool HGetObject(const std::string& key, const std::string& field, T& obj) {
        std::string value = HGet(key, field);
        if (value.empty()) {
            return false;
        }
        return Deserialize(value, obj);
    }

private:
    CacheManager() = default;
    ~CacheManager() { Shutdown(); }
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    void StartSyncThread();
    void StopSyncThread();
    void SyncThreadFunc();
    void OnSyncTimer();

    void UpdateAccessTime(const std::string& key);
    void UpdateModifyTime(const std::string& key);

    bool DoSyncToDB(const CacheEntry& entry);

    template<typename T>
    std::string Serialize(const T& obj) {
        if constexpr (std::is_same_v<T, std::string>) {
            return obj;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(obj);
        } else {
            if (serialize_callback_) {
                return serialize_callback_(typeid(T).name());
            }
            return "";
        }
    }

    template<typename T>
    bool Deserialize(const std::string& value, T& obj) {
        if constexpr (std::is_same_v<T, std::string>) {
            obj = value;
            return true;
        } else if constexpr (std::is_arithmetic_v<T>) {
            std::istringstream iss(value);
            iss >> obj;
            return !iss.fail();
        } else {
            if (deserialize_callback_) {
                deserialize_callback_(typeid(T).name(), value);
                return true;
            }
            return false;
        }
    }

    RedisConnectionPool::Ptr redis_pool_;
#ifdef USE_MYSQL
    std::shared_ptr<MySQLConnectionPool> mysql_pool_;
#endif

    CacheConfig config_;
    std::atomic<bool> initialized_{false};

    std::thread sync_thread_;
    std::mutex sync_mutex_;
    std::condition_variable sync_cond_;
    std::atomic<bool> stop_sync_{false};

    std::map<std::string, CacheEntry> local_cache_;
    std::set<std::string> dirty_keys_;
    mutable std::mutex cache_mutex_;

    SyncCallback sync_callback_;
    SerializeCallback serialize_callback_;
    DeserializeCallback deserialize_callback_;

    mutable CacheStats stats_;
    mutable std::mutex stats_mutex_;
};

class CacheTransaction {
public:
    explicit CacheTransaction(CacheManager& manager) 
        : manager_(manager), committed_(false) {}
    
    ~CacheTransaction() {
        if (!committed_) {
            Rollback();
        }
    }

    void Set(const std::string& key, const std::string& value,
             const std::string& table = "", const std::string& primary_key = "") {
        operations_.push_back([this, key, value, table, primary_key]() {
            manager_.Set(key, value, table, primary_key);
        });
    }

    void Del(const std::string& key) {
        operations_.push_back([this, key]() {
            manager_.Del(key);
        });
    }

    void MarkDirty(const std::string& key) {
        operations_.push_back([this, key]() {
            manager_.MarkDirty(key);
        });
    }

    bool Commit() {
        for (auto& op : operations_) {
            op();
        }
        committed_ = true;
        return true;
    }

    void Rollback() {
        operations_.clear();
    }

private:
    CacheManager& manager_;
    std::vector<std::function<void()>> operations_;
    bool committed_;
};

class CacheGuard {
public:
    explicit CacheGuard(const std::string& key, CacheManager& manager = CacheManager::Instance())
        : key_(key), manager_(manager) {
        value_ = manager_.Get(key_);
    }

    ~CacheGuard() {
        if (!value_.empty() && manager_.IsDirty(key_)) {
            manager_.SyncToDB(key_);
        }
    }

    const std::string& Get() const { return value_; }
    
    void Set(const std::string& value) {
        value_ = value;
        manager_.Set(key_, value_);
        manager_.MarkDirty(key_);
    }

    bool IsValid() const { return !value_.empty(); }

private:
    std::string key_;
    std::string value_;
    CacheManager& manager_;
};

}
