#include "service/game_service.h"
#include "script/lua_register.h"

namespace mmo {

REGISTER_SERVICE("game", GameService);

GameService::GameService(uint32_t id, const std::string& name)
    : ServiceBase(id, name) {
}

GameService::~GameService() {
    Shutdown();
}

bool GameService::Initialize() {
    LOG_INFO("Initializing GameService '{}'", name_);

    if (!lua_engine_.Initialize()) {
        LOG_ERROR("Failed to initialize Lua engine for GameService");
        return false;
    }

    RegisterLuaFunctions();

    std::string script_path = "lua/service/" + name_;
    lua_engine_.SetScriptPath(script_path);
    
    std::string init_script = script_path + "/init.lua";
    if (!lua_engine_.DoFile(init_script)) {
        LOG_WARN("GameService init script not found: {}", init_script);
    }

    running_ = true;
    LOG_INFO("GameService '{}' initialized successfully", name_);
    return true;
}

void GameService::Update(int64_t delta_ms) {
    ServiceMessage msg;
    while (PopMessage(msg)) {
        if (message_handler_) {
            message_handler_(msg);
        }

        lua_engine_.Call("on_service_message", msg.source, msg.type,
            std::string(msg.data.begin(), msg.data.end()));
    }

    lua_engine_.Call("on_update", delta_ms);
}

void GameService::Shutdown() {
    if (!running_) {
        return;
    }

    running_ = false;

    players_.clear();
    connection_to_player_.clear();

    lua_engine_.Shutdown();

    LOG_INFO("GameService '{}' shutdown", name_);
}

bool GameService::CreatePlayer(uint32_t player_id, uint32_t connection_id, const std::string& name) {
    if (players_.find(player_id) != players_.end()) {
        return false;
    }

    auto player = std::make_unique<GamePlayerInfo>();
    player->player_id = player_id;
    player->connection_id = connection_id;
    player->name = name;
    player->level = 1;
    player->hp = 100;
    player->mp = 100;
    player->x = 0.0f;
    player->y = 0.0f;
    player->z = 0.0f;

    connection_to_player_[connection_id] = player_id;
    players_[player_id] = std::move(player);

    LOG_INFO("Player created: {} (ID: {})", name, player_id);
    return true;
}

void GameService::RemovePlayer(uint32_t player_id) {
    auto it = players_.find(player_id);
    if (it != players_.end()) {
        connection_to_player_.erase(it->second->connection_id);
        LOG_INFO("Player removed: {} (ID: {})", it->second->name, player_id);
        players_.erase(it);
    }
}

GamePlayerInfo* GameService::GetPlayer(uint32_t player_id) {
    auto it = players_.find(player_id);
    if (it != players_.end()) {
        return it->second.get();
    }
    return nullptr;
}

GamePlayerInfo* GameService::GetPlayerByConnection(uint32_t connection_id) {
    auto it = connection_to_player_.find(connection_id);
    if (it != connection_to_player_.end()) {
        return GetPlayer(it->second);
    }
    return nullptr;
}

void GameService::UpdatePlayerPosition(uint32_t player_id, float x, float y, float z) {
    GamePlayerInfo* player = GetPlayer(player_id);
    if (player) {
        player->x = x;
        player->y = y;
        player->z = z;
    }
}

void GameService::UpdatePlayerHP(uint32_t player_id, int hp) {
    GamePlayerInfo* player = GetPlayer(player_id);
    if (player) {
        player->hp = hp;
    }
}

void GameService::UpdatePlayerMP(uint32_t player_id, int mp) {
    GamePlayerInfo* player = GetPlayer(player_id);
    if (player) {
        player->mp = mp;
    }
}

void GameService::RegisterLuaFunctions() {
    auto* self = this;

    luabridge::getGlobalNamespace(lua_engine_.GetState())
        .beginNamespace("game")
        .endNamespace();
}

}
