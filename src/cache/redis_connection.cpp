#include "cache/redis_connection.h"

#include <cstring>
#include <sstream>

extern "C" {
#include <hiredis.h>
}

namespace mmo {

RedisConnection::RedisConnection() = default;

RedisConnection::~RedisConnection() {
    Disconnect();
}

bool RedisConnection::Connect(const std::string& host, uint16_t port, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    host_ = host;
    port_ = port;
    password_ = password;
    
    context_ = redisConnect(host.c_str(), port);
    if (!context_ || context_->err) {
        LOG_ERROR("Redis connection failed: {}", context_ ? context_->errstr : "null");
        return false;
    }
    
    if (!password.empty()) {
        redisReply* reply = (redisReply*)redisCommand(context_, "AUTH %s", password.c_str());
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            LOG_ERROR("Redis authentication failed");
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
    }
    
    LOG_INFO("Connected to Redis at {}:{}", host, port);
    return true;
}

void RedisConnection::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
        LOG_INFO("Disconnected from Redis");
    }
}

bool RedisConnection::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return context_ != nullptr;
}

bool RedisConnection::Ping() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "PING");
    if (reply && reply->type == REDIS_REPLY_STATUS && 
        strcmp(reply->str, "PONG") == 0) {
        freeReplyObject(reply);
        return true;
    }
    freeReplyObject(reply);
    
    return false;
}

bool RedisConnection::Reconnect() {
    Disconnect();
    return Connect(host_, port_, password_);
}

bool RedisConnection::Set(const std::string& key, const std::string& value, int64_t ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply;
    if (ttl_seconds > 0) {
        reply = (redisReply*)redisCommand(context_, "SETEX %s %lld %s", 
                                       key.c_str(), ttl_seconds, value.c_str());
    } else {
        reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
    }
    
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        LOG_ERROR("Redis SET failed");
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

RedisResult RedisConnection::Get(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_NIL) {
        result.is_nil = true;
    } else if (reply->type == REDIS_REPLY_STRING) {
        result.value = std::string(reply->str, reply->len);
        result.success = true;
    } else if (reply->type == REDIS_REPLY_ERROR) {
        result.error_message = reply->str;
    }
    freeReplyObject(reply);
    
    return result;
}

bool RedisConnection::Del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::Exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "EXISTS %s", key.c_str());
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        bool exists = reply->integer > 0;
        freeReplyObject(reply);
        return exists;
    }
    freeReplyObject(reply);
    
    return false;
}

bool RedisConnection::Expire(const std::string& key, int64_t seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "EXPIRE %s %lld", 
                                               key.c_str(), seconds);
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        bool success = reply->integer == 1;
        freeReplyObject(reply);
        return success;
    }
    freeReplyObject(reply);
    
    return false;
}

int64_t RedisConnection::TTL(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return -2;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "TTL %s", key.c_str());
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        int64_t ttl = reply->integer;
        freeReplyObject(reply);
        return ttl;
    }
    freeReplyObject(reply);
    
    return -2;
}

// Hash operations
bool RedisConnection::HSet(const std::string& key, const std::string& field, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HSET %s %s %s",
                                               key.c_str(), field.c_str(), value.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

RedisResult RedisConnection::HGet(const std::string& key, const std::string& field) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HGET %s %s",
                                               key.c_str(), field.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_NIL) {
        result.is_nil = true;
    } else if (reply->type == REDIS_REPLY_STRING) {
        result.value = std::string(reply->str, reply->len);
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

bool RedisConnection::HDel(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HDEL %s %s",
                                               key.c_str(), field.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::HExists(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HEXISTS %s %s",
                                               key.c_str(), field.c_str());
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        bool exists = reply->integer == 1;
        freeReplyObject(reply);
        return exists;
    }
    freeReplyObject(reply);
    
    return false;
}

RedisResult RedisConnection::HGetAll(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "HGETALL %s", key.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i + 1 < reply->elements) {
                result.values.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
                result.values.push_back(std::string(reply->element[i+1]->str, reply->element[i+1]->len));
            }
        }
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

// List operations
bool RedisConnection::LPush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "LPUSH %s %s",
                                               key.c_str(), value.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::RPush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "RPUSH %s %s",
                                               key.c_str(), value.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

RedisResult RedisConnection::LPop(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "LPOP %s", key.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_NIL) {
        result.is_nil = true;
    } else if (reply->type == REDIS_REPLY_STRING) {
        result.value = std::string(reply->str, reply->len);
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

RedisResult RedisConnection::RPop(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "RPOP %s", key.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_NIL) {
        result.is_nil = true;
    } else if (reply->type == REDIS_REPLY_STRING) {
        result.value = std::string(reply->str, reply->len);
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

RedisResult RedisConnection::LLen(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "LLEN %s", key.c_str());
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        result.integer = reply->integer;
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

RedisResult RedisConnection::LRange(const std::string& key, int64_t start, int64_t stop) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "LRANGE %s %lld %lld",
                                               key.c_str(), start, stop);
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            result.values.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

// Set operations
bool RedisConnection::SAdd(const std::string& key, const std::string& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SADD %s %s",
                                               key.c_str(), member.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::SRem(const std::string& key, const std::string& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SREM %s %s",
                                               key.c_str(), member.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::SIsMember(const std::string& key, const std::string& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SISMEMBER %s %s",
                                               key.c_str(), member.c_str());
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        bool is_member = reply->integer == 1;
        freeReplyObject(reply);
        return is_member;
    }
    freeReplyObject(reply);
    
    return false;
}

RedisResult RedisConnection::SMembers(const std::string& key) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "SMEMBERS %s", key.c_str());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            result.values.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

// Sorted Set operations
bool RedisConnection::ZAdd(const std::string& key, double score, const std::string& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "ZADD %s %f %s",
                                               key.c_str(), score, member.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

bool RedisConnection::ZRem(const std::string& key, const std::string& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "ZREM %s %s",
                                               key.c_str(), member.c_str());
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

RedisResult RedisConnection::ZRange(const std::string& key, int64_t start, int64_t stop) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "ZRANGE %s %lld %lld",
                                               key.c_str(), start, stop);
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            result.values.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

RedisResult RedisConnection::ZRangeByScore(const std::string& key, double min, double max) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "ZRANGEBYSCORE %s %f %f",
                                               key.c_str(), min, max);
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            result.values.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
        result.success = true;
    }
    freeReplyObject(reply);
    
    return result;
}

// Generic operations
std::vector<std::string> RedisConnection::Keys(const std::string& pattern) {
    std::vector<std::string> keys;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return keys;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "KEYS %s", pattern.c_str());
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i++) {
            keys.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
    }
    freeReplyObject(reply);
    
    return keys;
}

bool RedisConnection::FlushDB() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "FLUSHDB");
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    return true;
}

RedisResult RedisConnection::Execute(const std::vector<std::string>& args) {
    RedisResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!context_) {
        result.error_message = "Not connected";
        return result;
    }
    
    // Build command
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
        argvlen.push_back(arg.length());
    }
    
    redisReply* reply = (redisReply*)redisCommandArgv(context_, args.size(), argv.data(), argvlen.data());
    if (!reply) {
        result.error_message = "Command failed";
        return result;
    }
    
    result = ParseReply(reply);
    freeReplyObject(reply);
    
    return result;
}

void RedisConnection::FreeReply(redisReply* reply) {
    freeReplyObject(reply);
}

RedisResult RedisConnection::ParseReply(redisReply* reply) {
    RedisResult result;
    
    if (!reply) {
        result.error_message = "Null reply";
        return result;
    }
    
    switch (reply->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
            result.value = std::string(reply->str, reply->len);
            result.success = true;
            break;
            
        case REDIS_REPLY_INTEGER:
            result.integer = reply->integer;
            result.success = true;
            break;
            
        case REDIS_REPLY_NIL:
            result.is_nil = true;
            result.success = true;
            break;
            
        case REDIS_REPLY_ARRAY:
            for (size_t i = 0; i < reply->elements; i++) {
                RedisResult element = ParseReply(reply->element[i]);
                if (element.success && !element.is_nil) {
                    result.values.push_back(element.value);
                }
            }
            result.success = true;
            break;
            
        case REDIS_REPLY_ERROR:
            result.error_message = reply->str;
            break;
            
        default:
            result.error_message = "Unknown reply type";
            break;
    }
    
    return result;
}

}
