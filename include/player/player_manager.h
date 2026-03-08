#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>

#include <google/protobuf/message.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "core/logger.h"
#include "player.pb.h"
#include "proto/lua_protobuf.h"

#ifdef USE_MYSQL

#include "db/mysql_pool.h"

namespace mmo {

class PlayerManager {
public:
    static PlayerManager& Instance() {
        static PlayerManager instance;
        return instance;
    }

    bool Initialize(const MySQLConfig& config) {
        if (!MySQLConnectionPool::Instance().Initialize(config)) {
            LOG_ERROR("Failed to initialize MySQL connection pool");
            return false;
        }

        LOG_INFO("PlayerManager initialized");
        return true;
    }

    void Shutdown() {
        MySQLConnectionPool::Instance().Shutdown();
        LOG_INFO("PlayerManager shutdown");
    }

    bool LoadPlayerFromDatabase(uint64_t player_id) {
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn) {
            LOG_ERROR("Failed to acquire MySQL connection");
            return false;
        }

        MySQLConnectionGuard guard(MySQLConnectionPool::Instance());
        
        std::string sql = "SELECT player_id, name, level, exp, hp, max_hp, mp, max_mp, "
                         "pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, profession, gender "
                         "FROM player WHERE player_id = ?";
        
        std::vector<std::string> params = {std::to_string(player_id)};
        MySQLResult result = conn->Query(sql, params);
        
        if (!result.success || result.rows.empty()) {
            LOG_ERROR("Failed to load player {} from database", player_id);
            return false;
        }

        const auto& row = result.rows[0];
        auto player_info = std::make_shared<mmo::PlayerInfo>();
        
        player_info->set_player_id(std::stoull(row[0]));
        player_info->set_name(row[1]);
        player_info->set_level(std::stoi(row[2]));
        player_info->set_exp(std::stoll(row[3]));
        player_info->set_hp(std::stoi(row[4]));
        player_info->set_max_hp(std::stoi(row[5]));
        player_info->set_mp(std::stoi(row[6]));
        player_info->set_max_mp(std::stoi(row[7]));
        
        auto position = player_info->mutable_position();
        position->set_x(std::stof(row[8]));
        position->set_y(std::stof(row[9]));
        position->set_z(std::stof(row[10]));
        
        auto rotation = player_info->mutable_rotation();
        rotation->set_x(std::stof(row[11]));
        rotation->set_y(std::stof(row[12]));
        rotation->set_z(std::stof(row[13]));
        
        player_info->set_profession(std::stoi(row[14]));
        player_info->set_gender(std::stoi(row[15]));

        std::lock_guard<std::mutex> lock(mutex_);
        players_[player_id] = player_info;
        
        LOG_INFO("Loaded player {} from database", player_id);
        return true;
    }

    bool LoadAllPlayersFromDatabase() {
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn) {
            LOG_ERROR("Failed to acquire MySQL connection");
            return false;
        }

        MySQLConnectionGuard guard(MySQLConnectionPool::Instance());
        
        std::string sql = "SELECT player_id, name, level, exp, hp, max_hp, mp, max_mp, "
                         "pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, profession, gender "
                         "FROM player";
        
        MySQLResult result = conn->Query(sql, {});
        
        if (!result.success) {
            LOG_ERROR("Failed to load players from database");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        players_.clear();
        
        for (const auto& row : result.rows) {
            auto player_info = std::make_shared<mmo::PlayerInfo>();
            
            player_info->set_player_id(std::stoull(row[0]));
            player_info->set_name(row[1]);
            player_info->set_level(std::stoi(row[2]));
            player_info->set_exp(std::stoll(row[3]));
            player_info->set_hp(std::stoi(row[4]));
            player_info->set_max_hp(std::stoi(row[5]));
            player_info->set_mp(std::stoi(row[6]));
            player_info->set_max_mp(std::stoi(row[7]));
            
            auto position = player_info->mutable_position();
            position->set_x(std::stof(row[8]));
            position->set_y(std::stof(row[9]));
            position->set_z(std::stof(row[10]));
            
            auto rotation = player_info->mutable_rotation();
            rotation->set_x(std::stof(row[11]));
            rotation->set_y(std::stof(row[12]));
            rotation->set_z(std::stof(row[13]));
            
            player_info->set_profession(std::stoi(row[14]));
            player_info->set_gender(std::stoi(row[15]));
            
            players_[player_info->player_id()] = player_info;
        }
        
        LOG_INFO("Loaded {} players from database", players_.size());
        return true;
    }

    bool SavePlayerToDatabase(uint64_t player_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = players_.find(player_id);
        if (it == players_.end()) {
            LOG_ERROR("Player {} not found in cache", player_id);
            return false;
        }

        auto player_info = it->second;
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn) {
            LOG_ERROR("Failed to acquire MySQL connection");
            return false;
        }

        MySQLConnectionGuard guard(MySQLConnectionPool::Instance());
        
        std::string sql = "UPDATE player SET name = ?, level = ?, exp = ?, hp = ?, max_hp = ?, "
                         "mp = ?, max_mp = ?, pos_x = ?, pos_y = ?, pos_z = ?, "
                         "rot_x = ?, rot_y = ?, rot_z = ?, profession = ?, gender = ? WHERE player_id = ?";
        
        std::vector<std::string> params = {
            player_info->name(),
            std::to_string(player_info->level()),
            std::to_string(player_info->exp()),
            std::to_string(player_info->hp()),
            std::to_string(player_info->max_hp()),
            std::to_string(player_info->mp()),
            std::to_string(player_info->max_mp()),
            std::to_string(player_info->position().x()),
            std::to_string(player_info->position().y()),
            std::to_string(player_info->position().z()),
            std::to_string(player_info->rotation().x()),
            std::to_string(player_info->rotation().y()),
            std::to_string(player_info->rotation().z()),
            std::to_string(player_info->profession()),
            std::to_string(player_info->gender()),
            std::to_string(player_id)
        };
        
        if (!conn->Execute(sql, params)) {
            LOG_ERROR("Failed to save player {} to database", player_id);
            return false;
        }

        LOG_INFO("Saved player {} to database", player_id);
        return true;
    }

    std::shared_ptr<mmo::PlayerInfo> GetPlayerInfo(uint64_t player_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = players_.find(player_id);
        if (it != players_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool SyncPlayerToLua(lua_State* L, uint64_t player_id) {
        auto player_info = GetPlayerInfo(player_id);
        if (!player_info) {
            LOG_ERROR("Player {} not found, cannot sync to Lua", player_id);
            return false;
        }

        std::string serialized_data = player_info->SerializeAsString();
        
        auto message = LuaProtobuf::Instance().ParseFromString("mmo.PlayerInfo", serialized_data);
        if (!message) {
            LOG_ERROR("Failed to parse protobuf message for player {}", player_id);
            return false;
        }

        auto lua_table = LuaProtobuf::Instance().MessageToLuaTable(*message);
        
        std::string player_id_str = std::to_string(player_id);
        
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mmo")
                .beginNamespace("player_cache")
                    .addVariable(player_id_str.c_str(), lua_table)
                .endNamespace()
            .endNamespace();

        LOG_INFO("Synced player {} to Lua", player_id);
        return true;
    }

    bool SyncAllPlayersToLua(lua_State* L) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& pair : players_) {
            uint64_t player_id = pair.first;
            auto player_info = pair.second;
            
            std::string serialized_data = player_info->SerializeAsString();
            
            auto message = LuaProtobuf::Instance().ParseFromString("mmo.PlayerInfo", serialized_data);
            if (!message) {
                LOG_ERROR("Failed to parse protobuf message for player {}", player_id);
                continue;
            }

            auto lua_table = LuaProtobuf::Instance().MessageToLuaTable(*message);
            
            std::string player_id_str = std::to_string(player_id);
            
            luabridge::getGlobalNamespace(L)
                .beginNamespace("mmo")
                    .beginNamespace("player_cache")
                        .addVariable(player_id_str.c_str(), lua_table)
                    .endNamespace()
                .endNamespace();
        }
        
        LOG_INFO("Synced {} players to Lua", players_.size());
        return true;
    }

    bool RemovePlayerFromCache(uint64_t player_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = players_.find(player_id);
        if (it != players_.end()) {
            players_.erase(it);
            LOG_INFO("Removed player {} from cache", player_id);
            return true;
        }
        return false;
    }

    size_t GetPlayerCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return players_.size();
    }

    bool InitializeDatabaseTables() {
        auto conn = MySQLConnectionPool::Instance().Acquire();
        if (!conn) {
            LOG_ERROR("Failed to acquire MySQL connection");
            return false;
        }

        MySQLConnectionGuard guard(MySQLConnectionPool::Instance());
        
        std::string sql = R"(
            CREATE TABLE IF NOT EXISTS player (
                player_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
                name VARCHAR(64) NOT NULL,
                level INT NOT NULL DEFAULT 1,
                exp BIGINT NOT NULL DEFAULT 0,
                hp INT NOT NULL DEFAULT 100,
                max_hp INT NOT NULL DEFAULT 100,
                mp INT NOT NULL DEFAULT 100,
                max_mp INT NOT NULL DEFAULT 100,
                pos_x FLOAT NOT NULL DEFAULT 0.0,
                pos_y FLOAT NOT NULL DEFAULT 0.0,
                pos_z FLOAT NOT NULL DEFAULT 0.0,
                rot_x FLOAT NOT NULL DEFAULT 0.0,
                rot_y FLOAT NOT NULL DEFAULT 0.0,
                rot_z FLOAT NOT NULL DEFAULT 0.0,
                profession INT NOT NULL DEFAULT 0,
                gender INT NOT NULL DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                PRIMARY KEY (player_id),
                UNIQUE KEY idx_name (name)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
        )";
        
        if (!conn->Execute(sql)) {
            LOG_ERROR("Failed to create player table");
            return false;
        }

        LOG_INFO("Database tables initialized");
        return true;
    }

private:
    PlayerManager() = default;
    ~PlayerManager() = default;
    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;

    std::unordered_map<uint64_t, std::shared_ptr<mmo::PlayerInfo>> players_;
    std::mutex mutex_;
};

}

#endif
