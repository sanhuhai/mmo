#include "player/weapon.h"
#include <random>
#include <sstream>

namespace mmo {

// WeaponEffect implementations

std::string FireDamageEffect::ToString() const {
    std::stringstream ss;
    ss << "Fire Damage: Deals " << damage_ << " fire damage over time for " << duration_ << " seconds";
    return ss.str();
}

void FireDamageEffect::Apply(uint64_t player_id) {
    active_ = true;
    LOG_INFO("Applied Fire Damage effect to player {}", player_id);
}

void FireDamageEffect::Remove(uint64_t player_id) {
    active_ = false;
    LOG_INFO("Removed Fire Damage effect from player {}", player_id);
}

std::string IceSlowEffect::ToString() const {
    std::stringstream ss;
    ss << "Ice Slow: Slows target by " << slow_percent_ * 100 << "% for " << duration_ << " seconds";
    return ss.str();
}

void IceSlowEffect::Apply(uint64_t player_id) {
    active_ = true;
    LOG_INFO("Applied Ice Slow effect to player {}", player_id);
}

void IceSlowEffect::Remove(uint64_t player_id) {
    active_ = false;
    LOG_INFO("Removed Ice Slow effect from player {}", player_id);
}

std::string LightningStrikeEffect::ToString() const {
    std::stringstream ss;
    ss << "Lightning Strike: Has " << chance_ * 100 << "% chance to strike with lightning for " << damage_ << " damage";
    return ss.str();
}

void LightningStrikeEffect::Apply(uint64_t player_id) {
    active_ = true;
    LOG_INFO("Applied Lightning Strike effect to player {}", player_id);
}

void LightningStrikeEffect::Remove(uint64_t player_id) {
    active_ = false;
    LOG_INFO("Removed Lightning Strike effect from player {}", player_id);
}

// Weapon implementations

Weapon::Weapon(uint64_t weapon_id, const std::string& name, WeaponType type, WeaponRarity rarity, int level)
    : weapon_id_(weapon_id),
      name_(name),
      type_(type),
      rarity_(rarity),
      level_(level) {
    CalculateAttributes();
}

void Weapon::AddEffect(std::shared_ptr<WeaponEffect> effect) {
    effects_.push_back(effect);
    LOG_INFO("Added effect '{}' to weapon {}", effect->GetName(), name_);
}

void Weapon::RemoveEffect(const std::string& effect_name) {
    auto it = std::remove_if(effects_.begin(), effects_.end(),
        [&effect_name](const std::shared_ptr<WeaponEffect>& effect) {
            return effect->GetName() == effect_name;
        });
    
    if (it != effects_.end()) {
        LOG_INFO("Removed effect '{}' from weapon {}", effect_name, name_);
        effects_.erase(it, effects_.end());
    }
}

void Weapon::CalculateAttributes() {
    // Reset base attributes
    attribute_ = WeaponAttribute();
    
    // Apply bonuses based on weapon type, rarity, and level
    ApplyTypeBonus();
    ApplyRarityBonus();
    ApplyLevelBonus();
    
    LOG_INFO("Calculated attributes for weapon {}: attack={}, defense={}, magic_power={}",
        name_, attribute_.attack, attribute_.defense, attribute_.magic_power);
}

void Weapon::ApplyRarityBonus() {
    float bonus_multiplier = 1.0f;
    
    switch (rarity_) {
        case mmo::WeaponRarity::COMMON:
            bonus_multiplier = 1.0f;
            break;
        case mmo::WeaponRarity::UNCOMMON:
            bonus_multiplier = 1.2f;
            break;
        case mmo::WeaponRarity::RARE:
            bonus_multiplier = 1.5f;
            break;
        case mmo::WeaponRarity::EPIC:
            bonus_multiplier = 2.0f;
            break;
        case mmo::WeaponRarity::LEGENDARY:
            bonus_multiplier = 3.0f;
            break;
    }
    
    attribute_.attack = static_cast<int>(attribute_.attack * bonus_multiplier);
    attribute_.defense = static_cast<int>(attribute_.defense * bonus_multiplier);
    attribute_.magic_power = static_cast<int>(attribute_.magic_power * bonus_multiplier);
    
    // Additional bonuses for higher rarity
    if (rarity_ >= mmo::WeaponRarity::RARE) {
        attribute_.critical_rate += 5;
        attribute_.critical_damage += 25;
    }
    
    if (rarity_ >= mmo::WeaponRarity::EPIC) {
        attribute_.attack_speed += 10;
        attribute_.accuracy += 5;
    }
    
    if (rarity_ == mmo::WeaponRarity::LEGENDARY) {
        attribute_.dodge += 5;
        attribute_.critical_rate += 5;
    }
}

void Weapon::ApplyLevelBonus() {
    int level_bonus = level_ * 2;
    attribute_.attack += level_bonus;
    attribute_.defense += level_bonus / 2;
    attribute_.magic_power += level_bonus;
    
    if (level_ > 50) {
        attribute_.critical_rate += (level_ - 50) / 10;
        attribute_.attack_speed += (level_ - 50) / 20;
    }
}

void Weapon::ApplyTypeBonus() {
    switch (type_) {
        case mmo::WeaponType::SWORD:
            attribute_.attack = 15;
            attribute_.defense = 5;
            attribute_.accuracy = 95;
            break;
        case mmo::WeaponType::AXE:
            attribute_.attack = 25;
            attribute_.defense = 3;
            attribute_.accuracy = 85;
            break;
        case mmo::WeaponType::BOW:
            attribute_.attack = 20;
            attribute_.defense = 2;
            attribute_.accuracy = 90;
            attribute_.attack_speed += 10;
            break;
        case mmo::WeaponType::WAND:
            attribute_.magic_power = 30;
            attribute_.attack = 5;
            attribute_.defense = 2;
            break;
        case mmo::WeaponType::DAGGER:
            attribute_.attack = 12;
            attribute_.defense = 2;
            attribute_.attack_speed += 20;
            attribute_.critical_rate += 10;
            break;
        case mmo::WeaponType::SHIELD:
            attribute_.defense = 20;
            attribute_.attack = 5;
            attribute_.dodge += 5;
            break;
        case mmo::WeaponType::GUN:
            attribute_.attack = 18;
            attribute_.defense = 1;
            attribute_.attack_speed += 15;
            attribute_.accuracy = 92;
            break;
        case mmo::WeaponType::STAFF:
            attribute_.magic_power = 25;
            attribute_.attack = 8;
            attribute_.defense = 3;
            break;
        case mmo::WeaponType::SPEAR:
            attribute_.attack = 22;
            attribute_.defense = 4;
            attribute_.accuracy = 88;
            break;
        case mmo::WeaponType::FIST:
            attribute_.attack = 10;
            attribute_.defense = 3;
            attribute_.attack_speed += 25;
            attribute_.critical_rate += 5;
            break;
    }
}

void Weapon::ApplyEffects(uint64_t player_id) {
    for (const auto& effect : effects_) {
        effect->Apply(player_id);
    }
}

void Weapon::RemoveEffects(uint64_t player_id) {
    for (const auto& effect : effects_) {
        effect->Remove(player_id);
    }
}

std::string Weapon::ToString() const {
    std::stringstream ss;
    ss << "Weapon: " << name_ << " (ID: " << weapon_id_ << ")" << std::endl;
    
    // Get weapon type name
    std::string type_name;
    switch (type_) {
        case mmo::WeaponType::SWORD: type_name = "Sword";
            break;
        case mmo::WeaponType::AXE: type_name = "Axe";
            break;
        case mmo::WeaponType::BOW: type_name = "Bow";
            break;
        case mmo::WeaponType::WAND: type_name = "Wand";
            break;
        case mmo::WeaponType::DAGGER: type_name = "Dagger";
            break;
        case mmo::WeaponType::SHIELD: type_name = "Shield";
            break;
        case mmo::WeaponType::GUN: type_name = "Gun";
            break;
        case mmo::WeaponType::STAFF: type_name = "Staff";
            break;
        case mmo::WeaponType::SPEAR: type_name = "Spear";
            break;
        case mmo::WeaponType::FIST: type_name = "Fist";
            break;
    }
    
    // Get rarity name
    std::string rarity_name;
    switch (rarity_) {
        case mmo::WeaponRarity::COMMON: rarity_name = "Common";
            break;
        case mmo::WeaponRarity::UNCOMMON: rarity_name = "Uncommon";
            break;
        case mmo::WeaponRarity::RARE: rarity_name = "Rare";
            break;
        case mmo::WeaponRarity::EPIC: rarity_name = "Epic";
            break;
        case mmo::WeaponRarity::LEGENDARY: rarity_name = "Legendary";
            break;
    }
    
    ss << "Type: " << type_name << ", Rarity: " << rarity_name << ", Level: " << level_ << std::endl;
    ss << "Attributes:" << std::endl;
    ss << "  Attack: " << attribute_.attack << std::endl;
    ss << "  Defense: " << attribute_.defense << std::endl;
    ss << "  Magic Power: " << attribute_.magic_power << std::endl;
    ss << "  Critical Rate: " << attribute_.critical_rate << "%" << std::endl;
    ss << "  Critical Damage: " << attribute_.critical_damage << "%" << std::endl;
    ss << "  Attack Speed: " << attribute_.attack_speed << "%" << std::endl;
    ss << "  Accuracy: " << attribute_.accuracy << "%" << std::endl;
    ss << "  Dodge: " << attribute_.dodge << "%" << std::endl;
    
    if (!effects_.empty()) {
        ss << "Effects:" << std::endl;
        for (const auto& effect : effects_) {
            ss << "  - " << effect->GetName() << ": " << effect->GetDescription() << std::endl;
        }
    }
    
    return ss.str();
}

// WeaponManager implementations

std::shared_ptr<Weapon> WeaponManager::CreateWeapon(uint64_t weapon_id, const std::string& name, WeaponType type, WeaponRarity rarity, int level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto weapon = std::make_shared<Weapon>(weapon_id, name, type, rarity, level);
    weapons_[weapon_id] = weapon;
    
    LOG_INFO("Created weapon: {} (ID: {})", name, weapon_id);
    
    return weapon;
}

std::shared_ptr<Weapon> WeaponManager::GetWeapon(uint64_t weapon_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = weapons_.find(weapon_id);
    if (it != weapons_.end()) {
        return it->second;
    }
    return nullptr;
}

bool WeaponManager::RemoveWeapon(uint64_t weapon_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = weapons_.find(weapon_id);
    if (it != weapons_.end()) {
        // Check if this weapon is equipped by any player
        for (auto& [player_id, equipped_weapon_id] : player_equipped_weapons_) {
            if (equipped_weapon_id == weapon_id) {
                UnequipWeapon(player_id);
            }
        }
        
        weapons_.erase(it);
        LOG_INFO("Removed weapon ID: {}", weapon_id);
        return true;
    }
    return false;
}

std::shared_ptr<Weapon> WeaponManager::GenerateRandomWeapon(int level, WeaponRarity rarity) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Generate random weapon ID
    uint64_t weapon_id = static_cast<uint64_t>(gen());
    
    // Random weapon type
    int type_index = gen() % 10;
    WeaponType type = static_cast<WeaponType>(type_index);
    
    // Generate weapon name
    std::string name = GenerateWeaponName(type, rarity);
    
    // Create weapon
    auto weapon = CreateWeapon(weapon_id, name, type, rarity, level);
    
    // Add random effects based on rarity
    int effect_count = 0;
    switch (rarity) {
        case WeaponRarity::COMMON:
            effect_count = 0;
            break;
        case WeaponRarity::UNCOMMON:
            effect_count = 1;
            break;
        case WeaponRarity::RARE:
            effect_count = 2;
            break;
        case WeaponRarity::EPIC:
            effect_count = 3;
            break;
        case WeaponRarity::LEGENDARY:
            effect_count = 4;
            break;
    }
    
    // Add random effects
    for (int i = 0; i < effect_count; ++i) {
        int effect_type = gen() % 3;
        switch (effect_type) {
            case 0: {
                int damage = 5 + (level / 10);
                int duration = 3 + (level / 20);
                weapon->AddEffect(std::make_shared<FireDamageEffect>(damage, duration));
                break;
            }
            case 1: {
                float slow = 0.1f + (level * 0.01f);
                int duration = 2 + (level / 25);
                weapon->AddEffect(std::make_shared<IceSlowEffect>(slow, duration));
                break;
            }
            case 2: {
                int damage = 10 + (level / 5);
                float chance = 0.05f + (level * 0.005f);
                weapon->AddEffect(std::make_shared<LightningStrikeEffect>(damage, chance));
                break;
            }
        }
    }
    
    LOG_INFO("Generated random weapon: {}", name);
    return weapon;
}

void WeaponManager::EquipWeapon(uint64_t player_id, uint64_t weapon_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto weapon = GetWeapon(weapon_id);
    if (!weapon) {
        LOG_ERROR("Weapon {} not found for player {}", weapon_id, player_id);
        return;
    }
    
    // Unequip existing weapon if any
    UnequipWeapon(player_id);
    
    // Equip new weapon
    player_equipped_weapons_[player_id] = weapon_id;
    weapon->ApplyEffects(player_id);
    
    LOG_INFO("Player {} equipped weapon: {}", player_id, weapon->GetName());
}

void WeaponManager::UnequipWeapon(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_equipped_weapons_.find(player_id);
    if (it != player_equipped_weapons_.end()) {
        auto weapon = GetWeapon(it->second);
        if (weapon) {
            weapon->RemoveEffects(player_id);
            LOG_INFO("Player {} unequipped weapon: {}", player_id, weapon->GetName());
        }
        player_equipped_weapons_.erase(it);
    }
}

std::shared_ptr<Weapon> WeaponManager::GetEquippedWeapon(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_equipped_weapons_.find(player_id);
    if (it != player_equipped_weapons_.end()) {
        return GetWeapon(it->second);
    }
    return nullptr;
}



void WeaponManager::InitializeWeaponNames() {
    // Initialize weapon names
    weapon_names_ = {
        "Blade", "Sword", "Axe", "Bow", "Wand", "Dagger", "Shield", "Gun", "Staff", "Spear", "Fist",
        "Edge", "Blade", "Hammer", "Crossbow", "Rod", "Knife", "Buckler", "Pistol", "Pole", "Lance", "Gauntlet",
        "Saber", "Claymore", "Mace", "Longbow", "Cane", "Stiletto", "Tower Shield", "Rifle", "Trident", "Javelin", "Brass Knuckles"
    };
    
    // Initialize weapon type names
    weapon_type_names_ = {
        {WeaponType::SWORD, "Sword"},
        {WeaponType::AXE, "Axe"},
        {WeaponType::BOW, "Bow"},
        {WeaponType::WAND, "Wand"},
        {WeaponType::DAGGER, "Dagger"},
        {WeaponType::SHIELD, "Shield"},
        {WeaponType::GUN, "Gun"},
        {WeaponType::STAFF, "Staff"},
        {WeaponType::SPEAR, "Spear"},
        {WeaponType::FIST, "Fist"}
    };
    
    // Initialize weapon rarity names
    weapon_rarity_names_ = {
        {WeaponRarity::COMMON, "Common"},
        {WeaponRarity::UNCOMMON, "Uncommon"},
        {WeaponRarity::RARE, "Rare"},
        {WeaponRarity::EPIC, "Epic"},
        {WeaponRarity::LEGENDARY, "Legendary"}
    };
}

std::string WeaponManager::GenerateWeaponName(WeaponType type, WeaponRarity rarity) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Get weapon type name
    auto type_name = weapon_type_names_[type];
    
    // Get rarity prefix
    std::string rarity_prefix;
    switch (rarity) {
        case WeaponRarity::COMMON:
            rarity_prefix = "";
            break;
        case WeaponRarity::UNCOMMON:
            rarity_prefix = "Fine ";
            break;
        case WeaponRarity::RARE:
            rarity_prefix = "Rare ";
            break;
        case WeaponRarity::EPIC:
            rarity_prefix = "Epic ";
            break;
        case WeaponRarity::LEGENDARY:
            rarity_prefix = "Legendary ";
            break;
    }
    
    // Get random adjective
    std::vector<std::string> adjectives = {
        "Sharp", "Mighty", "Swift", "Magical", "Ancient", "Enchanted", "Cursed", "Blessed",
        "Fiery", "Icy", "Thunderous", "Shadowy", "Radiant", "Vicious", "Graceful", "Stalwart"
    };
    
    std::string adjective = adjectives[gen() % adjectives.size()];
    
    // Get random weapon name
    std::string weapon_name = weapon_names_[gen() % weapon_names_.size()];
    
    // Combine to form full name
    std::stringstream ss;
    ss << rarity_prefix << adjective << " " << weapon_name;
    
    return ss.str();
}

} // namespace mmo
