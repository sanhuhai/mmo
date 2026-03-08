#pragma once

#include <unordered_map>
#include <memory>

#include "service/service_base.h"

namespace mmo {

struct GamePlayerInfo {
    uint32_t player_id;
    uint32_t connection_id;
    std::string name;
    int level;
    int hp;
    int mp;
    float x, y, z;
};

class GameService : public ServiceBase {
public:
    GameService(uint32_t id, const std::string& name);
    ~GameService() override;

    bool Initialize() override;
    void Update(int64_t delta_ms) override;
    void Shutdown() override;

    bool CreatePlayer(uint32_t player_id, uint32_t connection_id, const std::string& name);
    void RemovePlayer(uint32_t player_id);
    GamePlayerInfo* GetPlayer(uint32_t player_id);
    GamePlayerInfo* GetPlayerByConnection(uint32_t connection_id);

    void UpdatePlayerPosition(uint32_t player_id, float x, float y, float z);
    void UpdatePlayerHP(uint32_t player_id, int hp);
    void UpdatePlayerMP(uint32_t player_id, int mp);

    size_t GetPlayerCount() const { return players_.size(); }

private:
    void RegisterLuaFunctions();

    std::unordered_map<uint32_t, std::unique_ptr<GamePlayerInfo>> players_;
    std::unordered_map<uint32_t, uint32_t> connection_to_player_;
};

}
