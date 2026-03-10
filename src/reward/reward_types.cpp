#include "reward/reward_types.h"
#include <algorithm>

namespace mmo {
namespace reward {

ItemType StringToItemType(const std::string& str) {
    if (str == "currency") return ItemType::CURRENCY;
    if (str == "premium") return ItemType::PREMIUM;
    if (str == "consumable") return ItemType::CONSUMABLE;
    if (str == "weapon") return ItemType::WEAPON;
    if (str == "armor") return ItemType::ARMOR;
    if (str == "equipment") return ItemType::EQUIPMENT;
    if (str == "material") return ItemType::MATERIAL;
    if (str == "skillbook") return ItemType::SKILLBOOK;
    if (str == "gift") return ItemType::GIFT;
    if (str == "quest") return ItemType::QUEST;
    if (str == "special") return ItemType::SPECIAL;
    return ItemType::UNKNOWN;
}

std::string ItemTypeToString(ItemType type) {
    switch (type) {
        case ItemType::CURRENCY: return "currency";
        case ItemType::PREMIUM: return "premium";
        case ItemType::CONSUMABLE: return "consumable";
        case ItemType::WEAPON: return "weapon";
        case ItemType::ARMOR: return "armor";
        case ItemType::EQUIPMENT: return "equipment";
        case ItemType::MATERIAL: return "material";
        case ItemType::SKILLBOOK: return "skillbook";
        case ItemType::GIFT: return "gift";
        case ItemType::QUEST: return "quest";
        case ItemType::SPECIAL: return "special";
        default: return "unknown";
    }
}

ItemQuality IntToItemQuality(int32_t quality) {
    if (quality >= 1 && quality <= 5) {
        return static_cast<ItemQuality>(quality);
    }
    return ItemQuality::COMMON;
}

int32_t ItemQualityToInt(ItemQuality quality) {
    return static_cast<int32_t>(quality);
}

ItemEffectType IntToItemEffectType(int32_t type) {
    if (type == 0) return ItemEffectType::NONE;
    if (type == 1) return ItemEffectType::RESTORE_HP;
    if (type == 2) return ItemEffectType::RESTORE_MP;
    if (type == 3) return ItemEffectType::ADD_EXP;
    if (type == 4) return ItemEffectType::ADD_BUFF;
    if (type == 5) return ItemEffectType::LEARN_SKILL;
    if (type == 6) return ItemEffectType::TELEPORT;
    if (type == 7) return ItemEffectType::SUMMON;
    if (type >= 100) return ItemEffectType::CUSTOM;
    return ItemEffectType::NONE;
}

int32_t ItemEffectTypeToInt(ItemEffectType type) {
    return static_cast<int32_t>(type);
}

}
}
