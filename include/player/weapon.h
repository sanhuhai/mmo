#pragma once

#include "core/logger.h"

#include <string>
#include <vector>
#include <memory>
#include <random>
#include <unordered_map>
#include <mutex>
#include <algorithm>

namespace mmo {

struct WeaponAttribute {
    int attack;
    int defense;
    int magic_power;
    int critical_rate;
    int critical_damage;
    int attack_speed;
    int accuracy;
    int dodge;

    WeaponAttribute() :
        attack(0), defense(0), magic_power(0),
        critical_rate(0), critical_damage(150),
        attack_speed(100), accuracy(100), dodge(0) {}
};

enum class WeaponType {
    SWORD,
    AXE,
    BOW,
    WAND,
    DAGGER,
    SHIELD,
    GUN,
    STAFF,
    SPEAR,
    FIST
};

enum class WeaponRarity {
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY
};

class WeaponEffect : public std::enable_shared_from_this<WeaponEffect> {
public:
    WeaponEffect(const std::string& name, const std::string& description)
        : name_(name), description_(description), active_(false) {}
    virtual ~WeaponEffect() = default;

    const std::string& GetName() const { return name_; }
    const std::string& GetDescription() const { return description_; }
    bool IsActive() const { return active_; }

    virtual void Apply(uint64_t player_id) = 0;
    virtual void Remove(uint64_t player_id) = 0;
    virtual std::string ToString() const = 0;

protected:
    std::string name_;
    std::string description_;
    bool active_;
};

class FireDamageEffect : public WeaponEffect {
public:
    FireDamageEffect(int damage, int duration)
        : WeaponEffect("Fire Damage", "Deals fire damage over time"),
          damage_(damage), duration_(duration) {}

    void Apply(uint64_t player_id) override;
    void Remove(uint64_t player_id) override;
    std::string ToString() const override;

private:
    int damage_;
    int duration_;
};

class IceSlowEffect : public WeaponEffect {
public:
    IceSlowEffect(float slow_percent, int duration)
        : WeaponEffect("Ice Slow", "Slows enemy movement"),
          slow_percent_(slow_percent), duration_(duration) {}

    void Apply(uint64_t player_id) override;
    void Remove(uint64_t player_id) override;
    std::string ToString() const override;

private:
    float slow_percent_;
    int duration_;
};

class LightningStrikeEffect : public WeaponEffect {
public:
    LightningStrikeEffect(int damage, float chance)
        : WeaponEffect("Lightning Strike", "Chance to strike with lightning"),
          damage_(damage), chance_(chance) {}

    void Apply(uint64_t player_id) override;
    void Remove(uint64_t player_id) override;
    std::string ToString() const override;

private:
    int damage_;
    float chance_;
};

class Weapon : public std::enable_shared_from_this<Weapon> {
public:
    Weapon(uint64_t weapon_id, const std::string& name, WeaponType type, WeaponRarity rarity, int level);
    
    uint64_t GetWeaponId() const { return weapon_id_; }
    const std::string& GetName() const { return name_; }
    WeaponType GetType() const { return type_; }
    WeaponRarity GetRarity() const { return rarity_; }
    int GetLevel() const { return level_; }
    
    const WeaponAttribute& GetAttribute() const { return attribute_; }
    WeaponAttribute& GetAttribute() { return attribute_; }
    
    void AddEffect(std::shared_ptr<WeaponEffect> effect);
    void RemoveEffect(const std::string& effect_name);
    const std::vector<std::shared_ptr<WeaponEffect>>& GetEffects() const { return effects_; }
    
    void CalculateAttributes();
    void ApplyEffects(uint64_t player_id);
    void RemoveEffects(uint64_t player_id);
    
    std::string ToString() const;
private:
    uint64_t weapon_id_;
    std::string name_;
    WeaponType type_;
    WeaponRarity rarity_;
    int level_;
    WeaponAttribute attribute_;
    std::vector<std::shared_ptr<WeaponEffect>> effects_;
    
    void ApplyRarityBonus();
    void ApplyLevelBonus();
    void ApplyTypeBonus();
};

class WeaponManager {
public:
    static WeaponManager& Instance() {
        static WeaponManager instance;
        return instance;
    }

    std::shared_ptr<Weapon> CreateWeapon(uint64_t weapon_id, const std::string& name, WeaponType type, WeaponRarity rarity, int level);
    std::shared_ptr<Weapon> GetWeapon(uint64_t weapon_id);
    bool RemoveWeapon(uint64_t weapon_id);
    std::shared_ptr<Weapon> GenerateRandomWeapon(int level, WeaponRarity rarity);
    
    void EquipWeapon(uint64_t player_id, uint64_t weapon_id);
    void UnequipWeapon(uint64_t player_id);
    std::shared_ptr<Weapon> GetEquippedWeapon(uint64_t player_id);
    
    size_t GetWeaponCount() const { return weapons_.size(); }
    
private:
    WeaponManager() { InitializeWeaponNames(); }
    ~WeaponManager() = default;
    
    WeaponManager(const WeaponManager&) = delete;
    WeaponManager& operator=(const WeaponManager&) = delete;
    
    std::unordered_map<uint64_t, std::shared_ptr<Weapon>> weapons_;
    std::unordered_map<uint64_t, uint64_t> player_equipped_weapons_;
    mutable std::mutex mutex_;
    
    std::vector<std::string> weapon_names_;
    std::unordered_map<WeaponType, std::string> weapon_type_names_;
    std::unordered_map<WeaponRarity, std::string> weapon_rarity_names_;
    
    void InitializeWeaponNames();
    std::string GenerateWeaponName(WeaponType type, WeaponRarity rarity);
};

} // namespace mmo