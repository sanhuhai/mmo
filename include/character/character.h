#pragma once

#include <cstdint>
#include <string>

namespace mmo {
namespace character {

// Character class enumeration
enum CharacterClass {
    CHARACTER_CLASS_WARRIOR = 0,
    CHARACTER_CLASS_MAGE = 1,
    CHARACTER_CLASS_ARCHER = 2,
    CHARACTER_CLASS_ROGUE = 3,
    CHARACTER_CLASS_PRIEST = 4
};

// Character class with component system
class Character {
public:
    Character();
    ~Character();
    
    // Getters
    std::uint64_t GetCharacterId() const { return character_id_; }
    std::uint64_t GetPlayerId() const { return player_id_; }
    const std::string& GetName() const { return name_; }
    int GetCharacterClass() const { return character_class_; }
    int GetLevel() const { return level_; }
    int GetExperience() const { return experience_; }
    int GetHealth() const { return health_; }
    int GetMana() const { return mana_; }
    int GetStrength() const { return strength_; }
    int GetIntelligence() const { return intelligence_; }
    int GetAgility() const { return agility_; }
    int GetStamina() const { return stamina_; }
    std::uint32_t GetMapId() const { return map_id_; }
    float GetPositionX() const { return position_x_; }
    float GetPositionY() const { return position_y_; }
    float GetPositionZ() const { return position_z_; }
    std::int64_t GetCreatedTime() const { return created_time_; }
    std::int64_t GetLastLoginTime() const { return last_login_time_; }
    
    // Setters
    void SetCharacterId(std::uint64_t id) { character_id_ = id; }
    void SetPlayerId(std::uint64_t id) { player_id_ = id; }
    void SetName(const std::string& n) { name_ = n; }
    void SetCharacterClass(int cls) { character_class_ = cls; }
    void SetLevel(int lvl) { level_ = lvl; }
    void SetExperience(int exp) { experience_ = exp; }
    void SetHealth(int hp) { health_ = hp; }
    void SetMana(int mp) { mana_ = mp; }
    void SetStrength(int str) { strength_ = str; }
    void SetIntelligence(int int_) { intelligence_ = int_; }
    void SetAgility(int agi) { agility_ = agi; }
    void SetStamina(int sta) { stamina_ = sta; }
    void SetMapId(std::uint32_t map) { map_id_ = map; }
    void SetPosition(float x, float y, float z) { position_x_ = x; position_y_ = y; position_z_ = z; }
    void SetPositionX(float x) { position_x_ = x; }
    void SetPositionY(float y) { position_y_ = y; }
    void SetPositionZ(float z) { position_z_ = z; }
    void SetCreatedTime(std::int64_t time) { created_time_ = time; }
    void SetLastLoginTime(std::int64_t time) { last_login_time_ = time; }

private:
    std::uint64_t character_id_;
    std::uint64_t player_id_;
    std::string name_;
    int character_class_;
    int level_;
    int experience_;
    int health_;
    int mana_;
    int strength_;
    int intelligence_;
    int agility_;
    int stamina_;
    std::uint32_t map_id_;
    float position_x_;
    float position_y_;
    float position_z_;
    std::int64_t created_time_;
    std::int64_t last_login_time_;
};

} // namespace character
} // namespace mmo