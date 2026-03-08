#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>

#include "core/logger.h"

// Forward declaration for hiredis
struct redisContext;
struct redisReply;

namespace mmo {

struct RedisResult {
    bool success = false;
    std::string error_message;
    std::string value;
    std::vector<std::string> values;
    int64_t integer = 0;
    bool is_nil = false;
};

class RedisConnection {
public:
    using Ptr = std::shared_ptr<RedisConnection>;

    RedisConnection();
    ~RedisConnection();

    bool Connect(const std::string& host, uint16_t port, const std::string& password = "");
    void Disconnect();
    bool IsConnected() const;
    bool Ping();
    bool Reconnect();

    // String operations
    bool Set(const std::string& key, const std::string& value, int64_t ttl_seconds = 0);
    RedisResult Get(const std::string& key);
    bool Del(const std::string& key);
    bool Exists(const std::string& key);
    bool Expire(const std::string& key, int64_t seconds);
    int64_t TTL(const std::string& key);

    // Hash operations
    bool HSet(const std::string& key, const std::string& field, const std::string& value);
    RedisResult HGet(const std::string& key, const std::string& field);
    bool HDel(const std::string& key, const std::string& field);
    bool HExists(const std::string& key, const std::string& field);
    RedisResult HGetAll(const std::string& key);

    // List operations
    bool LPush(const std::string& key, const std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    RedisResult LPop(const std::string& key);
    RedisResult RPop(const std::string& key);
    RedisResult LLen(const std::string& key);
    RedisResult LRange(const std::string& key, int64_t start, int64_t stop);

    // Set operations
    bool SAdd(const std::string& key, const std::string& member);
    bool SRem(const std::string& key, const std::string& member);
    bool SIsMember(const std::string& key, const std::string& member);
    RedisResult SMembers(const std::string& key);

    // Sorted Set operations
    bool ZAdd(const std::string& key, double score, const std::string& member);
    bool ZRem(const std::string& key, const std::string& member);
    RedisResult ZRange(const std::string& key, int64_t start, int64_t stop);
    RedisResult ZRangeByScore(const std::string& key, double min, double max);

    // Generic operations
    std::vector<std::string> Keys(const std::string& pattern);
    bool FlushDB();
    RedisResult Execute(const std::vector<std::string>& args);

    const std::string& GetHost() const { return host_; }
    uint16_t GetPort() const { return port_; }

private:
    void FreeReply(redisReply* reply);
    RedisResult ParseReply(redisReply* reply);

    redisContext* context_ = nullptr;
    std::string host_;
    uint16_t port_ = 6379;
    std::string password_;
    
    mutable std::mutex mutex_;
};

}
