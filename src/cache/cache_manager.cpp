#include "cache/cache_manager.h"

#include <algorithm>
#include <sstream>

namespace mmo {

bool CacheManager::Initialize(RedisConnectionPool::Ptr redis_pool, const CacheConfig& config) {
    if (!redis_pool) {
        LOG_ERROR("Redis pool is required for cache manager");
        return false;
    }

    redis_pool_ = redis_pool;
    config_ = config;
    initialized_ = true;

    if (config_.auto_sync) {
        StartSyncThread();
    }

    LOG_INFO("CacheManager initialized: auto_sync={}, sync_interval={}s", 
             config_.auto_sync, config_.sync_interval.count());
    return true;
}

#ifdef USE_MYSQL
bool CacheManager::InitializeWithMySQL(RedisConnectionPool::Ptr redis_pool, 
                                        std::shared_ptr<MySQLConnectionPool> mysql_pool,
                                        const CacheConfig& config) {
    if (!Initialize(redis_pool, config)) {
        return false;
    }
    
    mysql_pool_ = mysql_pool;
    LOG_INFO("CacheManager initialized with MySQL support");
    return true;
}
#endif

void CacheManager::Shutdown() {
    StopSyncThread();

    if (config_.sync_on_shutdown) {
        LOG_INFO("Performing final sync before shutdown...");
        SyncAllToDB();
    }

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        local_cache_.clear();
        dirty_keys_.clear();
    }

    initialized_ = false;
    LOG_INFO("CacheManager shutdown complete");
}

void CacheManager::SetConfig(const CacheConfig& config) {
    bool need_restart_sync = (config_.auto_sync != config.auto_sync) || 
                              (config_.sync_interval != config.sync_interval);
    
    if (need_restart_sync && initialized_) {
        StopSyncThread();
    }

    config_ = config;

    if (need_restart_sync && initialized_ && config_.auto_sync) {
        StartSyncThread();
    }
}

void CacheManager::EnableAutoSync(bool enable) {
    if (config_.auto_sync == enable) {
        return;
    }

    config_.auto_sync = enable;

    if (initialized_) {
        if (enable) {
            StartSyncThread();
        } else {
            StopSyncThread();
        }
    }
}

void CacheManager::SetSyncInterval(std::chrono::seconds interval) {
    config_.sync_interval = interval;
    LOG_INFO("Sync interval updated to {} seconds", interval.count());
}

bool CacheManager::Set(const std::string& key, const std::string& value, 
                       const std::string& table, const std::string& primary_key,
                       int64_t ttl_seconds) {
    if (!initialized_) {
        LOG_ERROR("CacheManager not initialized");
        return false;
    }

    if (config_.enable_local_cache) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        CacheEntry entry;
        entry.key = key;
        entry.value = value;
        entry.table_name = table;
        entry.primary_key = primary_key;
        entry.last_access = std::chrono::steady_clock::now();
        entry.last_modify = std::chrono::steady_clock::now();
        entry.created_at = std::chrono::steady_clock::now();
        entry.dirty = !table.empty();
        entry.access_count = 1;
        entry.ttl_seconds = ttl_seconds;

        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            entry.created_at = it->second.created_at;
            if (!it->second.dirty && entry.dirty) {
                dirty_keys_.insert(key);
            }
        } else if (entry.dirty) {
            dirty_keys_.insert(key);
        }

        local_cache_[key] = entry;

        if (local_cache_.size() > config_.max_cache_size) {
            EvictLRU(local_cache_.size() - config_.max_cache_size);
        }
    }

    bool success = false;
    auto conn = redis_pool_->Acquire();
    if (conn) {
        success = conn->Set(key, value, ttl_seconds);
        redis_pool_->Release(conn);
    }

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

    std::string value;
    bool found = false;

    if (config_.enable_local_cache) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            it->second.last_access = std::chrono::steady_clock::now();
            it->second.access_count++;
            value = it->second.value;
            found = true;
        }
    }

    if (!found) {
        auto conn = redis_pool_->Acquire();
        if (conn) {
            auto result = conn->Get(key);
            redis_pool_->Release(conn);

            if (result.success && !result.is_nil) {
                value = result.value;
                found = true;

                if (config_.enable_local_cache) {
                    std::lock_guard<std::mutex> lock(cache_mutex_);
                    CacheEntry entry;
                    entry.key = key;
                    entry.value = value;
                    entry.last_access = std::chrono::steady_clock::now();
                    entry.created_at = std::chrono::steady_clock::now();
                    local_cache_[key] = entry;
                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (found) {
        stats_.hits++;
        stats_.reads++;
    } else {
        stats_.misses++;
    }

    double total = static_cast<double>(stats_.hits + stats_.misses);
    if (total > 0) {
        stats_.hit_rate = (static_cast<double>(stats_.hits) / total) * 100.0;
    }

    return value;
}

bool CacheManager::Del(const std::string& key) {
    if (!initialized_) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        local_cache_.erase(key);
        dirty_keys_.erase(key);
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool success = conn->Del(key);
    redis_pool_->Release(conn);
    return success;
}

bool CacheManager::Exists(const std::string& key) {
    if (!initialized_) {
        return false;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool exists = conn->Exists(key);
    redis_pool_->Release(conn);
    return exists;
}

bool CacheManager::SetEx(const std::string& key, const std::string& value, int64_t ttl_seconds) {
    return Set(key, value, "", "", ttl_seconds);
}

bool CacheManager::Expire(const std::string& key, int64_t ttl_seconds) {
    if (!initialized_) {
        return false;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool success = conn->Expire(key, ttl_seconds);
    redis_pool_->Release(conn);
    return success;
}

int64_t CacheManager::TTL(const std::string& key) {
    if (!initialized_) {
        return -1;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return -1;
    }

    int64_t ttl = conn->TTL(key);
    redis_pool_->Release(conn);
    return ttl;
}

bool CacheManager::HSet(const std::string& key, const std::string& field, 
                        const std::string& value,
                        const std::string& table, const std::string& primary_key) {
    if (!initialized_) {
        return false;
    }

    if (!table.empty() && config_.enable_local_cache) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            it->second.dirty = true;
            it->second.table_name = table;
            it->second.primary_key = primary_key;
            it->second.last_modify = std::chrono::steady_clock::now();
        } else {
            CacheEntry entry;
            entry.key = key;
            entry.table_name = table;
            entry.primary_key = primary_key;
            entry.last_access = std::chrono::steady_clock::now();
            entry.last_modify = std::chrono::steady_clock::now();
            entry.created_at = std::chrono::steady_clock::now();
            entry.dirty = true;
            local_cache_[key] = entry;
        }
        
        dirty_keys_.insert(key);
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool success = conn->HSet(key, field, value);
    redis_pool_->Release(conn);

    if (success) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.writes++;
    }

    return success;
}

std::string CacheManager::HGet(const std::string& key, const std::string& field) {
    if (!initialized_) {
        return "";
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return "";
    }

    auto result = conn->HGet(key, field);
    redis_pool_->Release(conn);

    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (result.success && !result.is_nil) {
        stats_.hits++;
        stats_.reads++;
        return result.value;
    }
    
    stats_.misses++;
    return "";
}

std::map<std::string, std::string> CacheManager::HGetAll(const std::string& key) {
    std::map<std::string, std::string> result;

    if (!initialized_) {
        return result;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return result;
    }

    auto reply = conn->HGetAll(key);
    redis_pool_->Release(conn);

    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (reply.success && reply.values.size() % 2 == 0) {
        for (size_t i = 0; i < reply.values.size(); i += 2) {
            result[reply.values[i]] = reply.values[i + 1];
        }
        stats_.hits++;
        stats_.reads++;
    } else {
        stats_.misses++;
    }

    return result;
}

bool CacheManager::HDel(const std::string& key, const std::string& field) {
    if (!initialized_) {
        return false;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool success = conn->HDel(key, field);
    redis_pool_->Release(conn);
    return success;
}

bool CacheManager::HExists(const std::string& key, const std::string& field) {
    if (!initialized_) {
        return false;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return false;
    }

    bool exists = conn->HExists(key, field);
    redis_pool_->Release(conn);
    return exists;
}

void CacheManager::MarkDirty(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        it->second.dirty = true;
        it->second.last_modify = std::chrono::steady_clock::now();
        dirty_keys_.insert(key);
    }
}

void CacheManager::MarkClean(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = local_cache_.find(key);
    if (it != local_cache_.end()) {
        it->second.dirty = false;
    }
    dirty_keys_.erase(key);
}

bool CacheManager::IsDirty(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = local_cache_.find(key);
    return it != local_cache_.end() && it->second.dirty;
}

bool CacheManager::SyncToDB(const std::string& key) {
#ifdef USE_MYSQL
    if (!mysql_pool_) {
        LOG_WARN("MySQL pool not available for sync");
        return false;
    }
#endif

    CacheEntry entry;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it == local_cache_.end()) {
            return true;
        }
        if (!it->second.dirty) {
            return true;
        }
        entry = it->second;
    }

    bool success = DoSyncToDB(entry);

    if (success) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = local_cache_.find(key);
        if (it != local_cache_.end()) {
            it->second.dirty = false;
        }
        dirty_keys_.erase(key);

        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.syncs++;
        LOG_DEBUG("Synced key '{}' to database", key);
    } else {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.sync_errors++;
        LOG_ERROR("Failed to sync key '{}' to database", key);
    }

    return success;
}

bool CacheManager::SyncAllToDB() {
    std::set<std::string> keys_to_sync;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        keys_to_sync = dirty_keys_;
    }

    if (keys_to_sync.empty()) {
        LOG_INFO("No dirty entries to sync");
        return true;
    }

    LOG_INFO("Starting full sync: {} dirty entries", keys_to_sync.size());

    int success_count = 0;
    int error_count = 0;

    for (const auto& key : keys_to_sync) {
        if (SyncToDB(key)) {
            success_count++;
        } else {
            error_count++;
        }
    }

    LOG_INFO("Full sync completed: {} succeeded, {} failed", success_count, error_count);
    return error_count == 0;
}

int CacheManager::SyncDirtyEntries() {
    std::set<std::string> keys_to_sync;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        keys_to_sync = dirty_keys_;
    }

    int success_count = 0;
    for (const auto& key : keys_to_sync) {
        if (SyncToDB(key)) {
            success_count++;
        }
    }

    return success_count;
}

size_t CacheManager::GetDirtyCount() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return dirty_keys_.size();
}

#ifdef USE_MYSQL
bool CacheManager::LoadFromMySQL(const std::string& table, const std::string& primary_key) {
    if (!mysql_pool_) {
        LOG_ERROR("MySQL pool not available");
        return false;
    }

    return mysql_pool_->Execute([this, &table, &primary_key](auto conn) {
        std::string sql = "SELECT * FROM " + table + " WHERE id = ?";
        auto result = conn->Query(sql, {primary_key});

        if (result.success && !result.rows.empty()) {
            std::string cache_key = table + ":" + primary_key;
            std::string value = "{}";
            
            if (sync_callback_) {
                sync_callback_(table, primary_key, value);
            }
            
            Set(cache_key, value, table, primary_key);
            return true;
        }

        return false;
    });
}

bool CacheManager::LoadQueryToCache(const std::string& sql, const std::string& table, 
                                     const std::string& key_column) {
    if (!mysql_pool_) {
        LOG_ERROR("MySQL pool not available");
        return false;
    }

    return mysql_pool_->Execute([this, &sql, &table, &key_column](auto conn) {
        auto result = conn->Query(sql);

        if (result.success) {
            for (const auto& row : result.rows) {
                if (!row.empty()) {
                    std::string cache_key = table + ":" + row[0];
                    std::string value = "{}";
                    Set(cache_key, value, table, row[0]);
                }
            }
            return true;
        }

        return false;
    });
}
#endif

void CacheManager::WarmCache(const std::string& table, const std::string& where_clause) {
#ifdef USE_MYSQL
    std::string sql = "SELECT * FROM " + table;
    if (!where_clause.empty()) {
        sql += " WHERE " + where_clause;
    }
    LoadQueryToCache(sql, table);
#else
    LOG_WARN("MySQL not enabled, cannot warm cache from database");
#endif
}

CacheStats CacheManager::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    CacheStats stats = stats_;
    stats.cache_size = local_cache_.size();
    stats.dirty_entries = dirty_keys_.size();
    
    return stats;
}

void CacheManager::ResetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = CacheStats{};
}

void CacheManager::PrintStats() const {
    auto stats = GetStats();

    LOG_INFO("========== Cache Statistics ==========");
    LOG_INFO("  Hits:       {}", stats.hits);
    LOG_INFO("  Misses:     {}", stats.misses);
    LOG_INFO("  Reads:      {}", stats.reads);
    LOG_INFO("  Writes:     {}", stats.writes);
    LOG_INFO("  Syncs:      {}", stats.syncs);
    LOG_INFO("  Sync Errors: {}", stats.sync_errors);
    LOG_INFO("  Evictions:  {}", stats.evictions);
    LOG_INFO("  Hit Rate:   {:.2f}%", stats.hit_rate);
    LOG_INFO("  Cache Size: {}", stats.cache_size);
    LOG_INFO("  Dirty Entries: {}", stats.dirty_entries);
    LOG_INFO("======================================");
}

void CacheManager::Clear() {
    ClearLocal();
    ClearRedis();
}

void CacheManager::ClearLocal() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    local_cache_.clear();
    dirty_keys_.clear();
    LOG_INFO("Local cache cleared");
}

void CacheManager::ClearRedis() {
    if (!initialized_) {
        return;
    }

    auto conn = redis_pool_->Acquire();
    if (conn) {
        conn->FlushDB();
        redis_pool_->Release(conn);
        LOG_INFO("Redis cache cleared");
    }
}

void CacheManager::EvictExpired() {
    std::vector<std::string> expired_keys;

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& [key, entry] : local_cache_) {
            auto conn = redis_pool_->Acquire();
            if (conn) {
                int64_t ttl = conn->TTL(key);
                redis_pool_->Release(conn);
                
                if (ttl == -2) {
                    expired_keys.push_back(key);
                }
            }
        }
    }

    if (!expired_keys.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& key : expired_keys) {
            local_cache_.erase(key);
            dirty_keys_.erase(key);
        }

        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.evictions += expired_keys.size();
    }
}

void CacheManager::EvictLRU(size_t count) {
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> access_times;

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& [key, entry] : local_cache_) {
            if (dirty_keys_.find(key) == dirty_keys_.end()) {
                access_times.emplace_back(key, entry.last_access);
            }
        }
    }

    std::sort(access_times.begin(), access_times.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });

    size_t to_evict = std::min(count, access_times.size());

    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (size_t i = 0; i < to_evict; ++i) {
            local_cache_.erase(access_times[i].first);
        }
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.evictions += to_evict;
    
    LOG_DEBUG("Evicted {} LRU entries", to_evict);
}

void CacheManager::EvictByPattern(const std::string& pattern) {
    if (!initialized_) {
        return;
    }

    auto conn = redis_pool_->Acquire();
    if (!conn) {
        return;
    }

    auto keys = conn->Keys(pattern);
    redis_pool_->Release(conn);

    size_t evicted = 0;
    for (const auto& key : keys) {
        if (Del(key)) {
            evicted++;
        }
    }

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.evictions += evicted;
    
    LOG_DEBUG("Evicted {} entries matching pattern '{}'", evicted, pattern);
}

void CacheManager::StartSyncThread() {
    if (sync_thread_.joinable()) {
        return;
    }

    stop_sync_ = false;
    sync_thread_ = std::thread(&CacheManager::SyncThreadFunc, this);
    LOG_INFO("Sync thread started with interval {}s", config_.sync_interval.count());
}

void CacheManager::StopSyncThread() {
    stop_sync_ = true;
    sync_cond_.notify_all();

    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }

    LOG_INFO("Sync thread stopped");
}

void CacheManager::SyncThreadFunc() {
    while (!stop_sync_) {
        std::unique_lock<std::mutex> lock(sync_mutex_);
        
        if (sync_cond_.wait_for(lock, config_.sync_interval, [this]() {
            return stop_sync_.load();
        })) {
            break;
        }

        OnSyncTimer();
    }
}

void CacheManager::OnSyncTimer() {
    LOG_INFO("Running scheduled cache sync...");

    auto start = std::chrono::steady_clock::now();
    int synced = SyncDirtyEntries();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    LOG_INFO("Scheduled sync completed: {} entries synced in {}ms", 
             synced, duration.count());

    EvictExpired();

    if (local_cache_.size() > config_.max_cache_size) {
        EvictLRU(local_cache_.size() - config_.max_cache_size);
    }
}

bool CacheManager::DoSyncToDB(const CacheEntry& entry) {
    if (entry.table_name.empty() || entry.primary_key.empty()) {
        return true;
    }

    if (sync_callback_) {
        return sync_callback_(entry.table_name, entry.primary_key, entry.value);
    }

#ifdef USE_MYSQL
    if (mysql_pool_) {
        return mysql_pool_->Execute([&entry](auto conn) {
            std::string check_sql = "SELECT id FROM " + entry.table_name + " WHERE id = ?";
            auto result = conn->Query(check_sql, {entry.primary_key});
            
            if (result.success && !result.rows.empty()) {
                return true;
            }
            return true;
        });
    }
#endif

    return true;
}

}
