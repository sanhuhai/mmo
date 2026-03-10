#pragma once

#include "reward_types.h"
#include <map>
#include <vector>
#include <functional>
#include <memory>

struct sqlite3;

namespace mmo {
namespace reward {

class RewardLoader {
public:
    using ProgressCallback = std::function<void(const std::string& table, int current, int total)>;
    
    RewardLoader();
    ~RewardLoader();
    
    bool Open(const std::string& db_path);
    void Close();
    
    bool LoadAllItems(std::map<int32_t, ItemConfig>& items);
    bool LoadAllRewards(std::map<int32_t, RewardConfig>& rewards);
    
    bool LoadItemsFromTable(const std::string& table_name, std::map<int32_t, ItemConfig>& items);
    bool LoadRewardsFromTable(const std::string& table_name, std::map<int32_t, RewardConfig>& rewards);
    
    const ItemConfig* GetItem(int32_t item_id) const;
    const RewardConfig* GetReward(int32_t reward_id) const;
    
    void SetProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }
    
    const std::string& GetLastError() const { return last_error_; }
    
    std::vector<std::string> GetTableNames();
    bool TableExists(const std::string& table_name);
    
    static ParsedReward ParseRewardString(const std::string& reward_str);
    static RewardItem ParseRewardItem(const std::string& item_str);
    
private:
    bool ParseItemRow(void* stmt, ItemConfig& item);
    bool ParseRewardRow(void* stmt, RewardConfig& reward);
    
    std::map<std::string, std::string> ParseExtraParams(const std::string& str);
    
private:
    sqlite3* db_;
    std::string last_error_;
    ProgressCallback progress_callback_;
    std::string db_path_;
    
    std::map<int32_t, ItemConfig> cached_items_;
    std::map<int32_t, RewardConfig> cached_rewards_;
};

class RewardManager {
public:
    static RewardManager& Instance();
    
    bool Initialize(const std::string& db_path);
    void Shutdown();
    
    bool LoadItems();
    bool LoadRewards();
    
    const ItemConfig* GetItem(int32_t item_id) const;
    const RewardConfig* GetReward(int32_t reward_id) const;
    
    ParsedReward GetParsedReward(int32_t reward_id) const;
    ParsedReward ParseReward(const std::string& reward_str) const;
    
    bool IsValidItem(int32_t item_id) const;
    bool IsValidReward(int32_t reward_id) const;
    
    size_t GetItemCount() const { return items_.size(); }
    size_t GetRewardCount() const { return rewards_.size(); }
    
    const std::map<int32_t, ItemConfig>& GetAllItems() const { return items_; }
    const std::map<int32_t, RewardConfig>& GetAllRewards() const { return rewards_; }
    
private:
    RewardManager() = default;
    ~RewardManager() = default;
    
    RewardManager(const RewardManager&) = delete;
    RewardManager& operator=(const RewardManager&) = delete;
    
private:
    RewardLoader loader_;
    std::map<int32_t, ItemConfig> items_;
    std::map<int32_t, RewardConfig> rewards_;
    bool initialized_ = false;
};

}
}
