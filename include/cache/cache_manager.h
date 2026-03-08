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

#include "cache/redis_pool.h"
#include "db/mysql_pool.h"
#include "utils/timer.h"
#include <lua.hpp>

namespace mmo {

// Cache entry metadata
struct CacheEntry {
    std::string key;
    std::string value;
    std::string table_name;
    std::string primary_key;
    std::chrono::steady_clock::time_point last_access;
    std::chrono::steady_clock::time_point created_at;
    bool dirty = false;
    int access_count = 0;
};

// Cache statistics
struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t writes = 0;
    uint64_t syncs = 0;
    uint64_t evictions = 0;
    size_t cache_size = 0;
    size_t dirty_entries = 0;
};

class CacheManager {
public:
    using Ptr = std::shared_ptr<CacheManager>;
    using SyncCallback = std::function<bool(const std::string& table, const std::string& key, const std::string& value)>;

    static CacheManager& Instance() {
        static CacheManager instance;
        return instance;
    }

    // Initialize with Redis and MySQL pools
    bool Initialize(RedisConnectionPool::Ptr redis_pool, 
                    std::shared_ptr<MySQLConnectionPool> mysql_pool = nullptr);
    void Shutdown();

    // Configuration
    void SetSyncInterval(std::chrono::seconds interval);
    void SetSyncCallback(SyncCallback callback);
    void EnableAutoSync(bool enable);

    // Cache operations
    bool Set(const std::string& key, const std::string& value, 
             const std::string& table = "", const std::string& primary_key = "",
             int64_t ttl_seconds = 0);
    std::string Get(const std::string& key);
    bool Del(const std::string& key);
    bool Exists(const std::string& key);

    // Hash operations for structured data
    bool HSet(const std::string& key, const std::string& field, const std::string& value,
              const std::string& table = "", const std::string& primary_key = "");
    std::string HGet(const std::string& key, const std::string& field);
    std::map<std::string, std::string> HGetAll(const std::string& key);
    bool HDel(const std::string& key, const std::string& field);

    // Mark entry as dirty (needs sync to MySQL)
    void MarkDirty(const std::string& key);
    bool IsDirty(const std::string& key);

    // Manual sync operations
    bool SyncToMySQL(const std::string& key);
    bool SyncAllToMySQL();
    int SyncDirtyEntries();

    // Load from MySQL to cache
    bool LoadFromMySQL(const std::string& table, const std::string& primary_key);
    bool LoadQueryToCache(const std::string& sql, const std::string& table);

    // Cache warming
    void WarmCache(const std::string& table, const std::string& where_clause = "");

    // Statistics
    CacheStats GetStats() const;
    void ResetStats();
    void PrintStats() const;

    // Maintenance
    void Clear();
    void EvictExpired();
    void EvictLRU(size_t count);

private:
    CacheManager() = default;
    ~CacheManager() { Shutdown(); }
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    void StartSyncTimer();
    void StopSyncTimer();
    void OnSyncTimer();

    void UpdateAccessTime(const std::string& key);
    void AddToDirtySet(const std::string& key);
    void RemoveFromDirtySet(const std::string& key);

    // Internal sync implementation
    bool DoSyncToMySQL(const CacheEntry& entry);
    std::string BuildInsertSQL(const std::string& table, const std::map<std::string, std::string>& data);
    std::string BuildUpdateSQL(const std::string& table, const std::string& primary_key,
                               const std::map<std::string, std::string>& data);

    RedisConnectionPool::Ptr redis_pool_;
    std::shared_ptr<MySQLConnectionPool> mysql_pool_;
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> auto_sync_{true};
    std::chrono::seconds sync_interval_{180}; // 3 minutes default
    
    std::unique_ptr<Timer> sync_timer_;
    std::thread sync_thread_;
    std::mutex sync_mutex_;
    
    // Local cache metadata
    std::map<std::string, CacheEntry> local_cache_;
    std::set<std::string> dirty_keys_;
    mutable std::mutex cache_mutex_;
    
    SyncCallback sync_callback_;
    CacheStats stats_;
    mutable std::mutex stats_mutex_;
};

// Lua bindings helper
class CacheLuaBinding {
public:
    static void Register(lua_State* L);

private:
    // Lua C functions
    static int LuaCacheSet(lua_State* L);
    static int LuaCacheGet(lua_State* L);
    static int LuaCacheDel(lua_State* L);
    static int LuaCacheHSet(lua_State* L);
    static int LuaCacheHGet(lua_State* L);
    static int LuaCacheHGetAll(lua_State* L);
    static int LuaCacheExists(lua_State* L);
    static int LuaCacheSync(lua_State* L);
    static int LuaCacheSyncAll(lua_State* L);
    static int LuaCacheLoadFromMySQL(lua_State* L);
    static int LuaCacheWarm(lua_State* L);
    static int LuaCacheStats(lua_State* L);
    static int LuaCacheClear(lua_State* L);
};

}
