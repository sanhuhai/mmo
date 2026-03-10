#pragma once

#include "reward_types.h"
#include "reward_loader.h"
#include <functional>
#include <vector>
#include <map>

namespace mmo {
namespace reward {

struct GiveResult {
    bool success = false;
    int32_t gold_added = 0;
    int32_t diamond_added = 0;
    int32_t exp_added = 0;
    std::vector<int32_t> items_added;
    std::string error_message;
};

struct GiveItemResult {
    bool success = false;
    int32_t item_id = 0;
    int32_t count = 0;
    std::string error_message;
};

class IRewardGiver {
public:
    virtual ~IRewardGiver() = default;
    
    virtual GiveResult GiveGold(uint64_t player_id, int32_t amount) = 0;
    virtual GiveResult GiveDiamond(uint64_t player_id, int32_t amount) = 0;
    virtual GiveResult GiveExp(uint64_t player_id, int32_t amount) = 0;
    virtual GiveItemResult GiveItem(uint64_t player_id, int32_t item_id, int32_t count) = 0;
    
    virtual bool CanGiveGold(uint64_t player_id, int32_t amount) { return amount > 0; }
    virtual bool CanGiveDiamond(uint64_t player_id, int32_t amount) { return amount > 0; }
    virtual bool CanGiveExp(uint64_t player_id, int32_t amount) { return amount > 0; }
    virtual bool CanGiveItem(uint64_t player_id, int32_t item_id, int32_t count);
    
    virtual GiveResult GiveReward(uint64_t player_id, const ParsedReward& reward) = 0;
    virtual GiveResult GiveRewardById(uint64_t player_id, int32_t reward_id) = 0;
    virtual GiveResult GiveRewardFromString(uint64_t player_id, const std::string& reward_str) = 0;
    
    virtual bool CanGiveReward(uint64_t player_id, const ParsedReward& reward) = 0;
    virtual bool CanGiveRewardById(uint64_t player_id, int32_t reward_id) = 0;
};

class RewardGiver : public IRewardGiver {
public:
    using GoldCallback = std::function<GiveResult(uint64_t, int32_t)>;
    using DiamondCallback = std::function<GiveResult(uint64_t, int32_t)>;
    using ExpCallback = std::function<GiveResult(uint64_t, int32_t)>;
    using ItemCallback = std::function<GiveItemResult(uint64_t, int32_t, int32_t)>;
    
    RewardGiver() = default;
    ~RewardGiver() override = default;
    
    GiveResult GiveGold(uint64_t player_id, int32_t amount) override;
    GiveResult GiveDiamond(uint64_t player_id, int32_t amount) override;
    GiveResult GiveExp(uint64_t player_id, int32_t amount) override;
    GiveItemResult GiveItem(uint64_t player_id, int32_t item_id, int32_t count) override;
    
    void SetGoldCallback(GoldCallback callback) { gold_callback_ = std::move(callback); }
    void SetDiamondCallback(DiamondCallback callback) { diamond_callback_ = std::move(callback); }
    void SetExpCallback(ExpCallback callback) { exp_callback_ = std::move(callback); }
    void SetItemCallback(ItemCallback callback) { item_callback_ = std::move(callback); }
    
    GiveResult GiveReward(uint64_t player_id, const ParsedReward& reward) override;
    GiveResult GiveRewardById(uint64_t player_id, int32_t reward_id) override;
    GiveResult GiveRewardFromString(uint64_t player_id, const std::string& reward_str) override;
    
    bool CanGiveReward(uint64_t player_id, const ParsedReward& reward) override;
    bool CanGiveRewardById(uint64_t player_id, int32_t reward_id) override;
    
private:
    GoldCallback gold_callback_;
    DiamondCallback diamond_callback_;
    ExpCallback exp_callback_;
    ItemCallback item_callback_;
};

class RewardService {
public:
    static RewardService& Instance();
    
    bool Initialize(const std::string& db_path);
    void Shutdown();
    
    void SetRewardGiver(IRewardGiver* giver) { giver_ = giver; }
    IRewardGiver* GetRewardGiver() { return giver_; }
    
    GiveResult GiveReward(uint64_t player_id, int32_t reward_id);
    GiveResult GiveRewardFromString(uint64_t player_id, const std::string& reward_str);
    GiveResult GiveParsedReward(uint64_t player_id, const ParsedReward& reward);
    
    GiveResult GiveGold(uint64_t player_id, int32_t amount);
    GiveResult GiveDiamond(uint64_t player_id, int32_t amount);
    GiveResult GiveExp(uint64_t player_id, int32_t amount);
    GiveItemResult GiveItem(uint64_t player_id, int32_t item_id, int32_t count);
    
    bool CanGiveReward(uint64_t player_id, int32_t reward_id);
    bool CanGiveReward(uint64_t player_id, const std::string& reward_str);
    
    const ItemConfig* GetItem(int32_t item_id) const;
    const RewardConfig* GetReward(int32_t reward_id) const;
    ParsedReward GetParsedReward(int32_t reward_id) const;
    
    bool IsValidItem(int32_t item_id) const;
    bool IsValidReward(int32_t reward_id) const;
    
private:
    RewardService() = default;
    ~RewardService() = default;
    
    RewardService(const RewardService&) = delete;
    RewardService& operator=(const RewardService&) = delete;
    
private:
    IRewardGiver* giver_ = nullptr;
    bool initialized_ = false;
};

}
}
