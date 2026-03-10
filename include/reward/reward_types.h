#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace mmo {
namespace reward {

enum class ItemType : uint8_t {
    UNKNOWN = 0,
    CURRENCY = 1,      
    PREMIUM = 2,       
    CONSUMABLE = 3,    
    WEAPON = 4,        
    ARMOR = 5,         
    EQUIPMENT = 6,     
    MATERIAL = 7,      
    SKILLBOOK = 8,     
    GIFT = 9,          
    QUEST = 10,        
    SPECIAL = 11       
};

enum class ItemQuality : uint8_t {
    COMMON = 1,        
    UNCOMMON = 2,      
    RARE = 3,          
    EPIC = 4,          
    LEGENDARY = 5      
};

enum class ItemEffectType : uint8_t {
    NONE = 0,
    RESTORE_HP = 1,        
    RESTORE_MP = 2,        
    ADD_EXP = 3,           
    ADD_BUFF = 4,          
    LEARN_SKILL = 5,       
    TELEPORT = 6,          
    SUMMON = 7,            
    CUSTOM = 100
};

struct ItemConfig {
    int32_t item_id = 0;
    std::string name;
    ItemType type = ItemType::UNKNOWN;
    std::string description;
    ItemQuality quality = ItemQuality::COMMON;
    int32_t max_stack = 1;
    std::string icon;
    
    ItemEffectType effect_type = ItemEffectType::NONE;
    int32_t effect_value = 0;
    int32_t effect_duration = 0;
    int32_t cooldown = 0;
    
    int32_t level_require = 1;
    int32_t vip_require = 0;
    
    bool tradeable = true;
    bool droppable = true;
    bool usable = false;
    
    int32_t expire_time = 0;
    
    std::map<std::string, std::string> extra_params;
    
    bool IsValid() const { return item_id > 0; }
    bool IsStackable() const { return max_stack > 1; }
    bool IsEquipment() const {
        return type == ItemType::WEAPON || 
               type == ItemType::ARMOR || 
               type == ItemType::EQUIPMENT;
    }
};

struct RewardConfig {
    int32_t reward_id = 0;
    std::string name;
    std::string rewards;
    std::string description;
    ItemQuality quality = ItemQuality::COMMON;
    std::string icon;
    
    bool IsValid() const { return reward_id > 0 && !rewards.empty(); }
};

struct RewardItem {
    enum class Type : uint8_t {
        GOLD = 0,
        DIAMOND = 1,
        EXP = 2,
        ITEM = 3,
        CUSTOM = 100
    };
    
    Type type = Type::GOLD;
    int32_t id = 0;      
    int32_t count = 0;   
    
    bool IsValid() const { return count > 0; }
};

struct ParsedReward {
    std::vector<RewardItem> items;
    int32_t total_gold = 0;
    int32_t total_diamond = 0;
    int32_t total_exp = 0;
    
    bool IsEmpty() const { return items.empty(); }
    bool HasGold() const { return total_gold > 0; }
    bool HasDiamond() const { return total_diamond > 0; }
    bool HasExp() const { return total_exp > 0; }
    bool HasItems() const;
};

inline bool ParsedReward::HasItems() const {
    for (const auto& item : items) {
        if (item.type == RewardItem::Type::ITEM) {
            return true;
        }
    }
    return false;
}

ItemType StringToItemType(const std::string& str);
std::string ItemTypeToString(ItemType type);

ItemQuality IntToItemQuality(int32_t quality);
int32_t ItemQualityToInt(ItemQuality quality);

ItemEffectType IntToItemEffectType(int32_t type);
int32_t ItemEffectTypeToInt(ItemEffectType type);

}
}
