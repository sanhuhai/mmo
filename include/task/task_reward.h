#pragma once

#include "task_data.h"
#include "task_manager.h"
#include <vector>
#include <map>
#include <functional>

namespace mmo {
namespace task {

struct RewardResult {
    bool success = false;
    int32_t gold_added = 0;
    int32_t exp_added = 0;
    int32_t diamond_added = 0;
    std::vector<int32_t> items_added;
    std::string error_message;
};

using RewardCallback = std::function<RewardResult(uint64_t player_id, const TaskReward& reward)>;

class DefaultRewardHandler : public ITaskRewardHandler {
public:
    DefaultRewardHandler();
    ~DefaultRewardHandler() override = default;
    
    bool GiveReward(uint64_t player_id, const TaskReward& reward) override;
    bool GiveRewards(uint64_t player_id, const std::vector<TaskReward>& rewards) override;
    bool CanGiveReward(uint64_t player_id, const TaskReward& reward) override;
    
    void SetGoldCallback(std::function<bool(uint64_t, int32_t)> callback) { gold_callback_ = callback; }
    void SetExpCallback(std::function<bool(uint64_t, int32_t)> callback) { exp_callback_ = callback; }
    void SetDiamondCallback(std::function<bool(uint64_t, int32_t)> callback) { diamond_callback_ = callback; }
    void SetItemCallback(std::function<bool(uint64_t, int32_t, int32_t)> callback) { item_callback_ = callback; }
    void SetCustomCallback(RewardCallback callback) { custom_callback_ = callback; }
    
    const RewardResult& GetLastResult() const { return last_result_; }
    
private:
    bool GiveGold(uint64_t player_id, int32_t amount);
    bool GiveExp(uint64_t player_id, int32_t amount);
    bool GiveDiamond(uint64_t player_id, int32_t amount);
    bool GiveItem(uint64_t player_id, int32_t item_id, int32_t count);
    bool GiveCustomReward(uint64_t player_id, const TaskReward& reward);
    
private:
    std::function<bool(uint64_t, int32_t)> gold_callback_;
    std::function<bool(uint64_t, int32_t)> exp_callback_;
    std::function<bool(uint64_t, int32_t)> diamond_callback_;
    std::function<bool(uint64_t, int32_t, int32_t)> item_callback_;
    RewardCallback custom_callback_;
    
    RewardResult last_result_;
};

class RewardManager {
public:
    static RewardManager& Instance();
    
    void Initialize();
    void Shutdown();
    
    void SetRewardHandler(ITaskRewardHandler* handler) { handler_ = handler; }
    ITaskRewardHandler* GetRewardHandler() { return handler_; }
    
    bool GiveReward(uint64_t player_id, const TaskReward& reward);
    bool GiveRewards(uint64_t player_id, const std::vector<TaskReward>& rewards);
    bool GiveRewardById(uint64_t player_id, int32_t reward_id);
    
    bool CanGiveReward(uint64_t player_id, const TaskReward& reward);
    
    void RegisterRewardTemplate(int32_t reward_id, const TaskReward& reward);
    const TaskReward* GetRewardTemplate(int32_t reward_id) const;
    
private:
    RewardManager() = default;
    ~RewardManager() = default;
    
    RewardManager(const RewardManager&) = delete;
    RewardManager& operator=(const RewardManager&) = delete;
    
private:
    ITaskRewardHandler* handler_ = nullptr;
    std::map<int32_t, TaskReward> reward_templates_;
    bool initialized_ = false;
};

}
}
