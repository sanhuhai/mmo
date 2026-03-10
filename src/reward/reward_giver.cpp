#include "reward/reward_giver.h"
#include "reward/reward_loader.h"

namespace mmo {
namespace reward {

bool IRewardGiver::CanGiveItem(uint64_t player_id, int32_t item_id, int32_t count) {
    auto* item_config = RewardManager::Instance().GetItem(item_id);
    if (!item_config) {
        return false;
    }
    return count > 0;
}

GiveResult RewardGiver::GiveGold(uint64_t player_id, int32_t amount) {
    GiveResult result;
    
    if (amount <= 0) {
        result.error_message = "Invalid gold amount";
        return result;
    }
    
    if (gold_callback_) {
        result = gold_callback_(player_id, amount);
    } else {
        result.success = true;
        result.gold_added = amount;
    }
    
    return result;
}

GiveResult RewardGiver::GiveDiamond(uint64_t player_id, int32_t amount) {
    GiveResult result;
    
    if (amount <= 0) {
        result.error_message = "Invalid diamond amount";
        return result;
    }
    
    if (diamond_callback_) {
        result = diamond_callback_(player_id, amount);
    } else {
        result.success = true;
        result.diamond_added = amount;
    }
    
    return result;
}

GiveResult RewardGiver::GiveExp(uint64_t player_id, int32_t amount) {
    GiveResult result;
    
    if (amount <= 0) {
        result.error_message = "Invalid exp amount";
        return result;
    }
    
    if (exp_callback_) {
        result = exp_callback_(player_id, amount);
    } else {
        result.success = true;
        result.exp_added = amount;
    }
    
    return result;
}

GiveItemResult RewardGiver::GiveItem(uint64_t player_id, int32_t item_id, int32_t count) {
    GiveItemResult result;
    result.item_id = item_id;
    result.count = count;
    
    if (item_id <= 0 || count <= 0) {
        result.error_message = "Invalid item id or count";
        return result;
    }
    
    auto* item_config = RewardManager::Instance().GetItem(item_id);
    if (!item_config) {
        result.error_message = "Item not found: " + std::to_string(item_id);
        return result;
    }
    
    if (item_callback_) {
        result = item_callback_(player_id, item_id, count);
    } else {
        result.success = true;
    }
    
    return result;
}

GiveResult RewardGiver::GiveReward(uint64_t player_id, const ParsedReward& reward) {
    GiveResult result;
    
    if (reward.IsEmpty()) {
        result.error_message = "Empty reward";
        return result;
    }
    
    if (reward.HasGold()) {
        auto gold_result = GiveGold(player_id, reward.total_gold);
        if (!gold_result.success) {
            result.error_message = "Failed to give gold: " + gold_result.error_message;
            return result;
        }
        result.gold_added = gold_result.gold_added;
    }
    
    if (reward.HasDiamond()) {
        auto diamond_result = GiveDiamond(player_id, reward.total_diamond);
        if (!diamond_result.success) {
            result.error_message = "Failed to give diamond: " + diamond_result.error_message;
            return result;
        }
        result.diamond_added = diamond_result.diamond_added;
    }
    
    if (reward.HasExp()) {
        auto exp_result = GiveExp(player_id, reward.total_exp);
        if (!exp_result.success) {
            result.error_message = "Failed to give exp: " + exp_result.error_message;
            return result;
        }
        result.exp_added = exp_result.exp_added;
    }
    
    for (const auto& item : reward.items) {
        if (item.type == RewardItem::Type::ITEM) {
            auto item_result = GiveItem(player_id, item.id, item.count);
            if (!item_result.success) {
                result.error_message = "Failed to give item " + std::to_string(item.id) + 
                                       ": " + item_result.error_message;
                return result;
            }
            result.items_added.push_back(item.id);
        }
    }
    
    result.success = true;
    return result;
}

GiveResult RewardGiver::GiveRewardById(uint64_t player_id, int32_t reward_id) {
    auto parsed = RewardManager::Instance().GetParsedReward(reward_id);
    if (parsed.IsEmpty()) {
        GiveResult result;
        result.error_message = "Reward not found or empty: " + std::to_string(reward_id);
        return result;
    }
    return GiveReward(player_id, parsed);
}

GiveResult RewardGiver::GiveRewardFromString(uint64_t player_id, const std::string& reward_str) {
    auto parsed = RewardLoader::ParseRewardString(reward_str);
    return GiveReward(player_id, parsed);
}

bool RewardGiver::CanGiveReward(uint64_t player_id, const ParsedReward& reward) {
    if (reward.IsEmpty()) return false;
    
    if (reward.HasGold() && !CanGiveGold(player_id, reward.total_gold)) {
        return false;
    }
    
    if (reward.HasDiamond() && !CanGiveDiamond(player_id, reward.total_diamond)) {
        return false;
    }
    
    if (reward.HasExp() && !CanGiveExp(player_id, reward.total_exp)) {
        return false;
    }
    
    for (const auto& item : reward.items) {
        if (item.type == RewardItem::Type::ITEM) {
            if (!CanGiveItem(player_id, item.id, item.count)) {
                return false;
            }
        }
    }
    
    return true;
}

bool RewardGiver::CanGiveRewardById(uint64_t player_id, int32_t reward_id) {
    auto parsed = RewardManager::Instance().GetParsedReward(reward_id);
    return CanGiveReward(player_id, parsed);
}

RewardService& RewardService::Instance() {
    static RewardService instance;
    return instance;
}

bool RewardService::Initialize(const std::string& db_path) {
    if (initialized_) return true;
    
    if (!RewardManager::Instance().Initialize(db_path)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void RewardService::Shutdown() {
    RewardManager::Instance().Shutdown();
    giver_ = nullptr;
    initialized_ = false;
}

GiveResult RewardService::GiveReward(uint64_t player_id, int32_t reward_id) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveRewardById(player_id, reward_id);
}

GiveResult RewardService::GiveRewardFromString(uint64_t player_id, const std::string& reward_str) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveRewardFromString(player_id, reward_str);
}

GiveResult RewardService::GiveParsedReward(uint64_t player_id, const ParsedReward& reward) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveReward(player_id, reward);
}

GiveResult RewardService::GiveGold(uint64_t player_id, int32_t amount) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveGold(player_id, amount);
}

GiveResult RewardService::GiveDiamond(uint64_t player_id, int32_t amount) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveDiamond(player_id, amount);
}

GiveResult RewardService::GiveExp(uint64_t player_id, int32_t amount) {
    if (!giver_) {
        GiveResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveExp(player_id, amount);
}

GiveItemResult RewardService::GiveItem(uint64_t player_id, int32_t item_id, int32_t count) {
    if (!giver_) {
        GiveItemResult result;
        result.error_message = "RewardGiver not set";
        return result;
    }
    return giver_->GiveItem(player_id, item_id, count);
}

bool RewardService::CanGiveReward(uint64_t player_id, int32_t reward_id) {
    if (!giver_) return false;
    return giver_->CanGiveRewardById(player_id, reward_id);
}

bool RewardService::CanGiveReward(uint64_t player_id, const std::string& reward_str) {
    if (!giver_) return false;
    auto parsed = RewardLoader::ParseRewardString(reward_str);
    return giver_->CanGiveReward(player_id, parsed);
}

const ItemConfig* RewardService::GetItem(int32_t item_id) const {
    return RewardManager::Instance().GetItem(item_id);
}

const RewardConfig* RewardService::GetReward(int32_t reward_id) const {
    return RewardManager::Instance().GetReward(reward_id);
}

ParsedReward RewardService::GetParsedReward(int32_t reward_id) const {
    return RewardManager::Instance().GetParsedReward(reward_id);
}

bool RewardService::IsValidItem(int32_t item_id) const {
    return RewardManager::Instance().IsValidItem(item_id);
}

bool RewardService::IsValidReward(int32_t reward_id) const {
    return RewardManager::Instance().IsValidReward(reward_id);
}

}
}
