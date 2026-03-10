#include "task/task_reward.h"

namespace mmo {
namespace task {

DefaultRewardHandler::DefaultRewardHandler() = default;

bool DefaultRewardHandler::GiveReward(uint64_t player_id, const TaskReward& reward) {
    last_result_ = RewardResult();
    
    if (reward.gold > 0) {
        if (!GiveGold(player_id, reward.gold)) {
            last_result_.error_message = "Failed to give gold";
            return false;
        }
        last_result_.gold_added = reward.gold;
    }
    
    if (reward.exp > 0) {
        if (!GiveExp(player_id, reward.exp)) {
            last_result_.error_message = "Failed to give exp";
            return false;
        }
        last_result_.exp_added = reward.exp;
    }
    
    if (reward.diamond > 0) {
        if (!GiveDiamond(player_id, reward.diamond)) {
            last_result_.error_message = "Failed to give diamond";
            return false;
        }
        last_result_.diamond_added = reward.diamond;
    }
    
    if (reward.item_id > 0 && reward.count > 0) {
        if (!GiveItem(player_id, reward.item_id, reward.count)) {
            last_result_.error_message = "Failed to give item";
            return false;
        }
        last_result_.items_added.push_back(reward.item_id);
    }
    
    if (!reward.custom_reward.empty()) {
        if (!GiveCustomReward(player_id, reward)) {
            last_result_.error_message = "Failed to give custom reward";
            return false;
        }
    }
    
    last_result_.success = true;
    return true;
}

bool DefaultRewardHandler::GiveRewards(uint64_t player_id, const std::vector<TaskReward>& rewards) {
    for (const auto& reward : rewards) {
        if (!GiveReward(player_id, reward)) {
            return false;
        }
    }
    return true;
}

bool DefaultRewardHandler::CanGiveReward(uint64_t player_id, const TaskReward& reward) {
    return true;
}

bool DefaultRewardHandler::GiveGold(uint64_t player_id, int32_t amount) {
    if (gold_callback_) {
        return gold_callback_(player_id, amount);
    }
    return true;
}

bool DefaultRewardHandler::GiveExp(uint64_t player_id, int32_t amount) {
    if (exp_callback_) {
        return exp_callback_(player_id, amount);
    }
    return true;
}

bool DefaultRewardHandler::GiveDiamond(uint64_t player_id, int32_t amount) {
    if (diamond_callback_) {
        return diamond_callback_(player_id, amount);
    }
    return true;
}

bool DefaultRewardHandler::GiveItem(uint64_t player_id, int32_t item_id, int32_t count) {
    if (item_callback_) {
        return item_callback_(player_id, item_id, count);
    }
    return true;
}

bool DefaultRewardHandler::GiveCustomReward(uint64_t player_id, const TaskReward& reward) {
    if (custom_callback_) {
        auto result = custom_callback_(player_id, reward);
        return result.success;
    }
    return true;
}

RewardManager& RewardManager::Instance() {
    static RewardManager instance;
    return instance;
}

void RewardManager::Initialize() {
    if (initialized_) return;
    
    initialized_ = true;
}

void RewardManager::Shutdown() {
    reward_templates_.clear();
    handler_ = nullptr;
    initialized_ = false;
}

bool RewardManager::GiveReward(uint64_t player_id, const TaskReward& reward) {
    if (!handler_) return false;
    return handler_->GiveReward(player_id, reward);
}

bool RewardManager::GiveRewards(uint64_t player_id, const std::vector<TaskReward>& rewards) {
    if (!handler_) return false;
    return handler_->GiveRewards(player_id, rewards);
}

bool RewardManager::GiveRewardById(uint64_t player_id, int32_t reward_id) {
    auto* reward = GetRewardTemplate(reward_id);
    if (!reward) return false;
    return GiveReward(player_id, *reward);
}

bool RewardManager::CanGiveReward(uint64_t player_id, const TaskReward& reward) {
    if (!handler_) return false;
    return handler_->CanGiveReward(player_id, reward);
}

void RewardManager::RegisterRewardTemplate(int32_t reward_id, const TaskReward& reward) {
    reward_templates_[reward_id] = reward;
}

const TaskReward* RewardManager::GetRewardTemplate(int32_t reward_id) const {
    auto it = reward_templates_.find(reward_id);
    if (it != reward_templates_.end()) {
        return &it->second;
    }
    return nullptr;
}

}
}
