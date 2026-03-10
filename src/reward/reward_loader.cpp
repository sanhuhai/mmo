#include "reward/reward_loader.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

namespace mmo {
namespace reward {

RewardLoader::RewardLoader()
    : db_(nullptr) {
}

RewardLoader::~RewardLoader() {
    Close();
}

bool RewardLoader::Open(const std::string& db_path) {
    Close();
    
    int result = sqlite3_open(db_path.c_str(), &db_);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to open database: " + db_path;
        return false;
    }
    
    db_path_ = db_path;
    return true;
}

void RewardLoader::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    db_path_.clear();
    cached_items_.clear();
    cached_rewards_.clear();
}

std::vector<std::string> RewardLoader::GetTableNames() {
    std::vector<std::string> tables;
    if (!db_) return tables;
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (name) {
                tables.push_back(name);
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return tables;
}

bool RewardLoader::TableExists(const std::string& table_name) {
    if (!db_) return false;
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

bool RewardLoader::LoadAllItems(std::map<int32_t, ItemConfig>& items) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    items.clear();
    
    if (TableExists("reward_item")) {
        LoadItemsFromTable("reward_item", items);
    }
    
    if (TableExists("items")) {
        LoadItemsFromTable("items", items);
    }
    
    if (TableExists("item")) {
        LoadItemsFromTable("item", items);
    }
    
    cached_items_ = items;
    return !items.empty();
}

bool RewardLoader::LoadAllRewards(std::map<int32_t, RewardConfig>& rewards) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    rewards.clear();
    
    if (TableExists("reward_config")) {
        LoadRewardsFromTable("reward_config", rewards);
    }
    
    if (TableExists("rewards")) {
        LoadRewardsFromTable("rewards", rewards);
    }
    
    if (TableExists("reward")) {
        LoadRewardsFromTable("reward", rewards);
    }
    
    cached_rewards_ = rewards;
    return !rewards.empty();
}

bool RewardLoader::LoadItemsFromTable(const std::string& table_name, std::map<int32_t, ItemConfig>& items) {
    if (!db_) return false;
    
    std::string sql = "SELECT * FROM \"" + table_name + "\";";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    int col_count = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ItemConfig item;
        
        for (int i = 0; i < col_count; ++i) {
            const char* col_name = sqlite3_column_name(stmt, i);
            std::string name = col_name ? col_name : "";
            
            if (name == "item_id" || name == "id") {
                item.item_id = sqlite3_column_int(stmt, i);
            } else if (name == "name") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                item.name = str ? str : "";
            } else if (name == "type" || name == "item_type") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                item.type = StringToItemType(str ? str : "");
            } else if (name == "description" || name == "desc") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                item.description = str ? str : "";
            } else if (name == "quality") {
                item.quality = IntToItemQuality(sqlite3_column_int(stmt, i));
            } else if (name == "max_stack" || name == "stack_limit") {
                item.max_stack = sqlite3_column_int(stmt, i);
            } else if (name == "icon") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                item.icon = str ? str : "";
            } else if (name == "effect_type") {
                item.effect_type = IntToItemEffectType(sqlite3_column_int(stmt, i));
            } else if (name == "effect_value") {
                item.effect_value = sqlite3_column_int(stmt, i);
            } else if (name == "effect_duration") {
                item.effect_duration = sqlite3_column_int(stmt, i);
            } else if (name == "cooldown" || name == "cd") {
                item.cooldown = sqlite3_column_int(stmt, i);
            } else if (name == "level_require" || name == "req_level") {
                item.level_require = sqlite3_column_int(stmt, i);
            } else if (name == "vip_require" || name == "req_vip") {
                item.vip_require = sqlite3_column_int(stmt, i);
            } else if (name == "tradeable") {
                item.tradeable = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "droppable") {
                item.droppable = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "usable") {
                item.usable = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "expire_time") {
                item.expire_time = sqlite3_column_int(stmt, i);
            } else if (name == "extra_params" || name == "extra") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (str) {
                    item.extra_params = ParseExtraParams(str);
                }
            }
        }
        
        if (item.item_id > 0) {
            items[item.item_id] = item;
        }
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool RewardLoader::LoadRewardsFromTable(const std::string& table_name, std::map<int32_t, RewardConfig>& rewards) {
    if (!db_) return false;
    
    std::string sql = "SELECT * FROM \"" + table_name + "\";";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    int col_count = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RewardConfig reward;
        
        for (int i = 0; i < col_count; ++i) {
            const char* col_name = sqlite3_column_name(stmt, i);
            std::string name = col_name ? col_name : "";
            
            if (name == "reward_id" || name == "id") {
                reward.reward_id = sqlite3_column_int(stmt, i);
            } else if (name == "name") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                reward.name = str ? str : "";
            } else if (name == "rewards" || name == "reward_list") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                reward.rewards = str ? str : "";
            } else if (name == "description" || name == "desc") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                reward.description = str ? str : "";
            } else if (name == "quality") {
                reward.quality = IntToItemQuality(sqlite3_column_int(stmt, i));
            } else if (name == "icon") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                reward.icon = str ? str : "";
            }
        }
        
        if (reward.reward_id > 0) {
            rewards[reward.reward_id] = reward;
        }
    }
    
    sqlite3_finalize(stmt);
    return true;
}

const ItemConfig* RewardLoader::GetItem(int32_t item_id) const {
    auto it = cached_items_.find(item_id);
    if (it != cached_items_.end()) {
        return &it->second;
    }
    return nullptr;
}

const RewardConfig* RewardLoader::GetReward(int32_t reward_id) const {
    auto it = cached_rewards_.find(reward_id);
    if (it != cached_rewards_.end()) {
        return &it->second;
    }
    return nullptr;
}

ParsedReward RewardLoader::ParseRewardString(const std::string& reward_str) {
    ParsedReward result;
    
    if (reward_str.empty()) {
        return result;
    }
    
    std::istringstream iss(reward_str);
    std::string item_str;
    
    while (std::getline(iss, item_str, ';')) {
        if (item_str.empty()) continue;
        
        RewardItem item = ParseRewardItem(item_str);
        if (item.IsValid()) {
            result.items.push_back(item);
            
            switch (item.type) {
                case RewardItem::Type::GOLD:
                    result.total_gold += item.count;
                    break;
                case RewardItem::Type::DIAMOND:
                    result.total_diamond += item.count;
                    break;
                case RewardItem::Type::EXP:
                    result.total_exp += item.count;
                    break;
                default:
                    break;
            }
        }
    }
    
    return result;
}

RewardItem RewardLoader::ParseRewardItem(const std::string& item_str) {
    RewardItem item;
    
    size_t pos = item_str.find(':');
    if (pos == std::string::npos) {
        return item;
    }
    
    std::string type_str = item_str.substr(0, pos);
    std::string value_str = item_str.substr(pos + 1);
    
    if (type_str == "gold") {
        item.type = RewardItem::Type::GOLD;
        item.count = std::stoi(value_str);
    } else if (type_str == "diamond") {
        item.type = RewardItem::Type::DIAMOND;
        item.count = std::stoi(value_str);
    } else if (type_str == "exp") {
        item.type = RewardItem::Type::EXP;
        item.count = std::stoi(value_str);
    } else if (type_str == "item") {
        item.type = RewardItem::Type::ITEM;
        size_t pos2 = value_str.find(':');
        if (pos2 != std::string::npos) {
            item.id = std::stoi(value_str.substr(0, pos2));
            item.count = std::stoi(value_str.substr(pos2 + 1));
        } else {
            item.id = std::stoi(value_str);
            item.count = 1;
        }
    } else {
        item.type = RewardItem::Type::CUSTOM;
        item.id = 0;
        item.count = std::stoi(value_str);
    }
    
    return item;
}

std::map<std::string, std::string> RewardLoader::ParseExtraParams(const std::string& str) {
    std::map<std::string, std::string> params;
    
    std::istringstream iss(str);
    std::string pair;
    
    while (std::getline(iss, pair, ',')) {
        size_t pos = pair.find(':');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;
        }
    }
    
    return params;
}

bool RewardLoader::ParseItemRow(void* stmt, ItemConfig& item) {
    return true;
}

bool RewardLoader::ParseRewardRow(void* stmt, RewardConfig& reward) {
    return true;
}

RewardManager& RewardManager::Instance() {
    static RewardManager instance;
    return instance;
}

bool RewardManager::Initialize(const std::string& db_path) {
    if (initialized_) return true;
    
    if (!loader_.Open(db_path)) {
        return false;
    }
    
    if (!LoadItems() || !LoadRewards()) {
        loader_.Close();
        return false;
    }
    
    initialized_ = true;
    return true;
}

void RewardManager::Shutdown() {
    loader_.Close();
    items_.clear();
    rewards_.clear();
    initialized_ = false;
}

bool RewardManager::LoadItems() {
    return loader_.LoadAllItems(items_);
}

bool RewardManager::LoadRewards() {
    return loader_.LoadAllRewards(rewards_);
}

const ItemConfig* RewardManager::GetItem(int32_t item_id) const {
    auto it = items_.find(item_id);
    if (it != items_.end()) {
        return &it->second;
    }
    return nullptr;
}

const RewardConfig* RewardManager::GetReward(int32_t reward_id) const {
    auto it = rewards_.find(reward_id);
    if (it != rewards_.end()) {
        return &it->second;
    }
    return nullptr;
}

ParsedReward RewardManager::GetParsedReward(int32_t reward_id) const {
    auto* reward = GetReward(reward_id);
    if (reward) {
        return RewardLoader::ParseRewardString(reward->rewards);
    }
    return ParsedReward();
}

ParsedReward RewardManager::ParseReward(const std::string& reward_str) const {
    return RewardLoader::ParseRewardString(reward_str);
}

bool RewardManager::IsValidItem(int32_t item_id) const {
    return items_.find(item_id) != items_.end();
}

bool RewardManager::IsValidReward(int32_t reward_id) const {
    return rewards_.find(reward_id) != rewards_.end();
}

}
}
