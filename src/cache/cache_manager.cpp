#include "cache/cache_manager.h"

#include <algorithm>
#include <sstream>
#include <json/json.h>

namespace mmo {

bool CacheManager::Initialize(RedisConnectionPool::Ptr redis_pool, 
                               std::shared_ptr<MySQLConnectionPool> mysql_pool) {
    redis_pool_ = redis_pool;
    mysql_pool_ = mysql_pool;
    
    if (!redis_pool_) {
        LOG_ERROR("Redis pool is required for cache manager");
        return false;
    }
    
    initialized_ = true;
    
    if (auto_sync_) {
        StartSyncTimer();
    }
    
    LOG_INFO("CacheManager initialized, auto_sync={}", auto_sync_);
    return true;
}

void CacheManager::Shutdown() {
    StopSyncTimer();
    
    if (auto_sync_) {
        // Final sync before shutdown
        SyncAllToMySQL();
    }
    
    initialized_ = false;
    LOG_INFO("CacheManager shutdown");
}

void CacheManager::SetSyncInterval(std::chrono::seconds interval) {
    sync_interval_ = interval;
    
    if (sync_timer_) {
        StopSyncTimer();
        StartSyncTimer();
    }
}

void CacheManager::SetSyncCallback(SyncCallback callback) {
    sync_callback_ = callback;
}

void CacheManager::EnableAutoSync(bool enable) {
    auto_sync_ = enable;
    
    if (enable && initialized_) {
        StartSyncTimer();
    } else {
        StopSyncTimer();
    }
}

bool CacheManager::Set(const std::string& key, const std::string& value, 
                       const std::string& table, const std::string& primary_key,
                       int64_t ttl_seconds) {
    if (!initialized_) {
        return false;
    }
    
    // Update local cache metadata
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        CacheEntry entry;
        entry.key = key;
        entry.value = value;
        entry.table_name = table;
        entry.primary_key = primary_key;
        entry.last_access = std::chrono::steady_clock::now();
        entry.created_at = std::chrono::steady_clock::now();
        entry.dirty = !table.empty();
        entry.access_count = 1;
        
        local_cache_[key] = entry;
        
        if (entry.dirty) {
            dirty_keys_.insert(key);
        }
    }
    
    // Write to Redis
    bool success = RedisConnectionGuard(*redis_pool_)->Set(key, value, ttl_seconds);
    
    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.writes++;
    }
    
    return success;
}

std::string CacheManager::Get(const std::string& key) {
    if (!initialized_) {
        return "";
    }
    
    // Try Redis first
    auto result = RedisConnectionGuard(*redis_pool_)->Get(key);
    
    if (result.success && !result.is_nil) {
        UpdateAccessTime(key);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.hits++;
        
        return result.value;
    }
    
    // Cache miss
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.misses++;
    }
    
    return "";
}

bool CacheManager::Del(const std::string& key) {
    if (!initialized_) {
        return false;
    }
    
    // Remove from local cache
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        local_cache_.erase(key);
        dirty_keys_.erase(key);
    }
    
    return RedisConnectionGuard(*redis_pool_)->Del(key);
}

bool CacheManager::Exists(const std::string& key) {
    if (!initialized_) {
        return false;
    }
    
    return RedisConnectionGuard(*redis_pool_)->Exists(key);
}

bool CacheManager::HSet(const std::string& key, const std::string& field, 
                        const std::string& value,
                        const std::string& table, const std::string& primary_key) {
    if (!initialized_) {
        return false;
    }
    
    // Mark as dirty if associated with a table
    if (!table.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            it->second.dirty = true;
            it->second.table_name = table;
            it->second.primary_key = primary_key;
            dirty_keys_.insert(key);
        } else {
            CacheEntry entry;
            entry.key = key;
            entry.table_name = table;
            entry.primary_key = primary_key;
            entry.last_access = std::chrono::steady_clock::now();
            entry.created_at = std::chrono::steady_clock::now();
            entry.dirty = true;
            local_cache_[key] = entry;
            dirty_keys_.insert(key);
        }
    }
    
    return RedisConnectionGuard(*redis_pool_)->HSet(key, field, value);
}

std::string CacheManager::HGet(const std::string& key, const std::string& field) {
    if (!initialized_) {
        return "";
    }
    
    auto result = RedisConnectionGuard(*redis_pool_)->HGet(key, field);
    
    if (result.success && !result.is_nil) {
        UpdateAccessTime(key);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.hits++;
        
        return result.value;
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.misses++;
    
    return "";
}

std::map<std::string, std::string> CacheManager::HGetAll(const std::string& key) {
    std::map<std::string, std::string> result;
    
    if (!initialized_) {
        return result;
    }
    
    auto reply = RedisConnectionGuard(*redis_pool_)->HGetAll(key);
    
    if (reply.success && reply.values.size() % 2 == 0) {
        for (size_t i = 0; i < reply.values.size(); i += 2) {
            result[reply.values[i]] = reply.values[i + 1];
        }
        
        UpdateAccessTime(key);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.hits++;
    } else {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.misses++;
    }
    
    return result;
}

bool CacheManager::HDel(const std::string& key, const std::string& field) {
    if (!initialized_) {
        return false;
    }
    
    return RedisConnectionGuard(*redis_pool_)->HDel(key, field);
}

void CacheManager::MarkDirty(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        it->second.dirty = true;
        dirty_keys_.insert(key);
    }
}

bool CacheManager::IsDirty(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = local_cache_.find(key);
    return it != local_cache_.end() && it->second.dirty;
}

bool CacheManager::SyncToMySQL(const std::string& key) {
    if (!mysql_pool_) {
        LOG_WARN("MySQL pool not available for sync");
        return false;
    }
    
    CacheEntry entry;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it == local_cache_.end() || !it->second.dirty) {
            return true; // Nothing to sync
        }
        entry = it->second;
    }
    
    bool success = DoSyncToMySQL(entry);
    
    if (success) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            it->second.dirty = false;
        }
        dirty_keys_.erase(key);
        
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.syncs++;
    }
    
    return success;
}

bool CacheManager::SyncAllToMySQL() {
    if (!mysql_pool_) {
        return false;
    }
    
    std::set<std::string> keys_to_sync;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        keys_to_sync = dirty_keys_;
    }
    
    int success_count = 0;
    for (const auto& key : keys_to_sync) {
        if (SyncToMySQL(key)) {
            success_count++;
        }
    }
    
    LOG_INFO("Synced {}/{} dirty entries to MySQL", success_count, keys_to_sync.size());
    return success_count == static_cast<int>(keys_to_sync.size());
}

int CacheManager::SyncDirtyEntries() {
    std::set<std::string> keys_to_sync;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        keys_to_sync = dirty_keys_;
    }
    
    int success_count = 0;
    for (const auto& key : keys_to_sync) {
        if (SyncToMySQL(key)) {
            success_count++;
        }
    }
    
    return success_count;
}

bool CacheManager::LoadFromMySQL(const std::string& table, const std::string& primary_key) {
    if (!mysql_pool_) {
        return false;
    }
    
    return MySQLConnectionGuard(*mysql_pool_).Execute([this, &table, &primary_key](auto conn) {
        std::string sql = "SELECT * FROM " + table + " WHERE id = ?";
        auto result = conn->Query(sql, {primary_key});
        
        if (result.success && !result.rows.empty()) {
            // Convert row to JSON and cache
            Json::Value json_data;
            for (size_t i = 0; i < result.columns.size(); ++i) {
                json_data[result.columns[i]] = result.rows[0][i];
            }
            
            Json::StreamWriterBuilder builder;
            std::string json_str = Json::writeString(builder, json_data);
            
            std::string cache_key = table + ":" + primary_key;
            Set(cache_key, json_str, table, primary_key);
            
            return true;
        }
        
        return false;
    });
}

bool CacheManager::LoadQueryToCache(const std::string& sql, const std::string& table) {
    if (!mysql_pool_) {
        return false;
    }
    
    return MySQLConnectionGuard(*mysql_pool_).Execute([this, &sql, &table](auto conn) {
        auto result = conn->Query(sql);
        
        if (result.success) {
            for (const auto& row : result.rows) {
                Json::Value json_data;
                for (size_t i = 0; i < result.columns.size(); ++i) {
                    json_data[result.columns[i]] = row[i];
                }
                
                Json::StreamWriterBuilder builder;
                std::string json_str = Json::writeString(builder, json_data);
                
                // Use first column as primary key
                std::string cache_key = table + ":" + row[0];
                Set(cache_key, json_str, table, row[0]);
            }
            
            return true;
        }
        
        return false;
    });
}

void CacheManager::WarmCache(const std::string& table, const std::string& where_clause) {
    std::string sql = "SELECT * FROM " + table;
    if (!where_clause.empty()) {
        sql += " WHERE " + where_clause;
    }
    
    LoadQueryToCache(sql, table);
}

CacheStats CacheManager::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void CacheManager::ResetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = CacheStats();
}

void CacheManager::PrintStats() const {
    auto stats = GetStats();
    
    LOG_INFO("=== Cache Statistics ===");
    LOG_INFO("  Hits: {}", stats.hits);
    LOG_INFO("  Misses: {}", stats.misses);
    LOG_INFO("  Writes: {}", stats.writes);
    LOG_INFO("  Syncs: {}", stats.syncs);
    LOG_INFO("  Evictions: {}", stats.evictions);
    LOG_INFO("  Hit Rate: {:.2f}%", 
             stats.hits + stats.misses > 0 
             ? (100.0 * stats.hits / (stats.hits + stats.misses)) 
             : 0.0);
    LOG_INFO("========================");
}

void CacheManager::Clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    local_cache_.clear();
    dirty_keys_.clear();
    
    RedisConnectionGuard(*redis_pool_)->FlushDB();
}

void CacheManager::EvictExpired() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> expired_keys;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& [key, entry] : local_cache_) {
            // Check if expired (TTL = -2 means expired)
            auto ttl_result = RedisConnectionGuard(*redis_pool_)->TTL(key);
            if (ttl_result.integer == -2) {
                expired_keys.push_back(key);
            }
        }
    }
    
    for (const auto& key : expired_keys) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        local_cache_.erase(key);
        dirty_keys_.erase(key);
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.evictions += expired_keys.size();
}

void CacheManager::EvictLRU(size_t count) {
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> access_times;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& [key, entry] : local_cache_) {
            access_times.emplace_back(key, entry.last_access);
        }
    }
    
    // Sort by access time (oldest first)
    std::sort(access_times.begin(), access_times.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });
    
    size_t to_evict = std::min(count, access_times.size());
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (size_t i = 0; i < to_evict; ++i) {
            local_cache_.erase(access_times[i].first);
            dirty_keys_.erase(access_times[i].first);
        }
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.evictions += to_evict;
}

void CacheManager::StartSyncTimer() {
    sync_thread_ = std::thread([this]() {
        while (initialized_ && auto_sync_) {
            std::this_thread::sleep_for(sync_interval_);
            
            if (initialized_ && auto_sync_) {
                OnSyncTimer();
            }
        }
    });
}

void CacheManager::StopSyncTimer() {
    auto_sync_ = false;
    
    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }
}

void CacheManager::OnSyncTimer() {
    LOG_INFO("Running scheduled cache sync to MySQL...");
    
    int synced = SyncDirtyEntries();
    
    LOG_INFO("Scheduled sync completed: {} entries synced", synced);
}

void CacheManager::UpdateAccessTime(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        it->second.last_access = std::chrono::steady_clock::now();
        it->second.access_count++;
    }
}

void CacheManager::AddToDirtySet(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    dirty_keys_.insert(key);
}

void CacheManager::RemoveFromDirtySet(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    dirty_keys_.erase(key);
}

bool CacheManager::DoSyncToMySQL(const CacheEntry& entry) {
    if (entry.table_name.empty() || entry.primary_key.empty()) {
        return true; // Nothing to sync
    }
    
    if (sync_callback_) {
        return sync_callback_(entry.table_name, entry.primary_key, entry.value);
    }
    
    // Default sync: parse JSON and update MySQL
    Json::Value json_data;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::istringstream json_stream(entry.value);
    if (!Json::parseFromStream(builder, json_stream, &json_data, &errors)) {
        LOG_ERROR("Failed to parse cache entry JSON: {}", errors);
        return false;
    }
    
    // Build and execute UPDATE SQL
    std::string sql = BuildUpdateSQL(entry.table_name, entry.primary_key, 
                                      {{"data", entry.value}});
    
    return MySQLConnectionGuard(*mysql_pool_).Execute([&sql](auto conn) {
        return conn->Execute(sql);
    });
}

std::string CacheManager::BuildInsertSQL(const std::string& table, 
                                          const std::map<std::string, std::string>& data) {
    std::stringstream sql;
    sql << "INSERT INTO " << table << " (";
    
    bool first = true;
    for (const auto& [column, _] : data) {
        if (!first) sql << ", ";
        sql << column;
        first = false;
    }
    
    sql << ") VALUES (";
    
    first = true;
    for (const auto& [_, value] : data) {
        if (!first) sql << ", ";
        sql << "'" << value << "'"; // TODO: Proper escaping
        first = false;
    }
    
    sql << ")";
    
    return sql.str();
}

std::string CacheManager::BuildUpdateSQL(const std::string& table, 
                                          const std::string& primary_key,
                                          const std::map<std::string, std::string>& data) {
    std::stringstream sql;
    sql << "UPDATE " << table << " SET ";
    
    bool first = true;
    for (const auto& [column, value] : data) {
        if (!first) sql << ", ";
        sql << column << " = '" << value << "'"; // TODO: Proper escaping
        first = false;
    }
    
    sql << " WHERE id = '" << primary_key << "'";
    
    return sql.str();
}

// ==================== Lua Bindings ====================

void CacheLuaBinding::Register(lua_State* L) {
    lua_newtable(L);
    
    lua_pushcfunction(L, LuaCacheSet);
    lua_setfield(L, -2, "set");
    
    lua_pushcfunction(L, LuaCacheGet);
    lua_setfield(L, -2, "get");
    
    lua_pushcfunction(L, LuaCacheDel);
    lua_setfield(L, -2, "del");
    
    lua_pushcfunction(L, LuaCacheHSet);
    lua_setfield(L, -2, "hset");
    
    lua_pushcfunction(L, LuaCacheHGet);
    lua_setfield(L, -2, "hget");
    
    lua_pushcfunction(L, LuaCacheHGetAll);
    lua_setfield(L, -2, "hgetall");
    
    lua_pushcfunction(L, LuaCacheExists);
    lua_setfield(L, -2, "exists");
    
    lua_pushcfunction(L, LuaCacheSync);
    lua_setfield(L, -2, "sync");
    
    lua_pushcfunction(L, LuaCacheSyncAll);
    lua_setfield(L, -2, "sync_all");
    
    lua_pushcfunction(L, LuaCacheLoadFromMySQL);
    lua_setfield(L, -2, "load_from_mysql");
    
    lua_pushcfunction(L, LuaCacheWarm);
    lua_setfield(L, -2, "warm");
    
    lua_pushcfunction(L, LuaCacheStats);
    lua_setfield(L, -2, "stats");
    
    lua_pushcfunction(L, LuaCacheClear);
    lua_setfield(L, -2, "clear");
    
    lua_setglobal(L, "cache");
}

int CacheLuaBinding::LuaCacheSet(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    const char* value = luaL_checkstring(L, 2);
    const char* table = luaL_optstring(L, 3, "");
    const char* primary_key = luaL_optstring(L, 4, "");
    int64_t ttl = luaL_optinteger(L, 5, 0);
    
    bool success = CacheManager::Instance().Set(key, value, table, primary_key, ttl);
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheGet(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    
    std::string value = CacheManager::Instance().Get(key);
    if (value.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, value.c_str());
    }
    return 1;
}

int CacheLuaBinding::LuaCacheDel(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    
    bool success = CacheManager::Instance().Del(key);
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheHSet(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    const char* field = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    const char* table = luaL_optstring(L, 4, "");
    const char* primary_key = luaL_optstring(L, 5, "");
    
    bool success = CacheManager::Instance().HSet(key, field, value, table, primary_key);
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheHGet(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    const char* field = luaL_checkstring(L, 2);
    
    std::string value = CacheManager::Instance().HGet(key, field);
    if (value.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, value.c_str());
    }
    return 1;
}

int CacheLuaBinding::LuaCacheHGetAll(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    
    auto result = CacheManager::Instance().HGetAll(key);
    
    lua_newtable(L);
    for (const auto& [field, value] : result) {
        lua_pushstring(L, value.c_str());
        lua_setfield(L, -2, field.c_str());
    }
    return 1;
}

int CacheLuaBinding::LuaCacheExists(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    
    bool exists = CacheManager::Instance().Exists(key);
    lua_pushboolean(L, exists);
    return 1;
}

int CacheLuaBinding::LuaCacheSync(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    
    bool success = CacheManager::Instance().SyncToMySQL(key);
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheSyncAll(lua_State* L) {
    bool success = CacheManager::Instance().SyncAllToMySQL();
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheLoadFromMySQL(lua_State* L) {
    const char* table = luaL_checkstring(L, 1);
    const char* primary_key = luaL_checkstring(L, 2);
    
    bool success = CacheManager::Instance().LoadFromMySQL(table, primary_key);
    lua_pushboolean(L, success);
    return 1;
}

int CacheLuaBinding::LuaCacheWarm(lua_State* L) {
    const char* table = luaL_checkstring(L, 1);
    const char* where_clause = luaL_optstring(L, 2, "");
    
    CacheManager::Instance().WarmCache(table, where_clause);
    lua_pushboolean(L, true);
    return 1;
}

int CacheLuaBinding::LuaCacheStats(lua_State* L) {
    auto stats = CacheManager::Instance().GetStats();
    
    lua_newtable(L);
    
    lua_pushinteger(L, stats.hits);
    lua_setfield(L, -2, "hits");
    
    lua_pushinteger(L, stats.misses);
    lua_setfield(L, -2, "misses");
    
    lua_pushinteger(L, stats.writes);
    lua_setfield(L, -2, "writes");
    
    lua_pushinteger(L, stats.syncs);
    lua_setfield(L, -2, "syncs");
    
    lua_pushinteger(L, stats.evictions);
    lua_setfield(L, -2, "evictions");
    
    double hit_rate = (stats.hits + stats.misses > 0) 
                      ? (100.0 * stats.hits / (stats.hits + stats.misses)) 
                      : 0.0;
    lua_pushnumber(L, hit_rate);
    lua_setfield(L, -2, "hit_rate");
    
    return 1;
}

int CacheLuaBinding::LuaCacheClear(lua_State* L) {
    CacheManager::Instance().Clear();
    lua_pushboolean(L, true);
    return 1;
}

}
