#include "role/character_manager.h"
#include <chrono>
#include <iostream>

namespace mmo {
namespace role {

Character::Character()
    : character_id_(0)
    , player_id_(0)
    , name_("")
    , character_class_(CHARACTER_CLASS_WARRIOR)
    , level_(1)
    , experience_(0)
    , health_(100)
    , mana_(50)
    , strength_(10)
    , intelligence_(10)
    , agility_(10)
    , stamina_(10)
    , map_id_(1)
    , position_x_(0.0f)
    , position_y_(0.0f)
    , position_z_(0.0f)
    , created_time_(0)
    , last_login_time_(0) {
}

Character::~Character() {
}

CharacterManager::CharacterManager() : initialized_(false), next_character_id_(1) {
}

CharacterManager::~CharacterManager() {
}

CharacterManager& CharacterManager::Instance() {
    static CharacterManager instance;
    return instance;
}

bool CharacterManager::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return true;
    }
    
    initialized_ = true;
    return true;
}

void CharacterManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }
    
    characters_.clear();
    player_characters_.clear();
    next_character_id_ = 1;
    initialized_ = false;
}

bool CharacterManager::CreateCharacter(std::uint64_t player_id, const std::string& name, int character_class) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Character character;
    character.SetCharacterId(GenerateCharacterId());
    character.SetPlayerId(player_id);
    character.SetName(name);
    character.SetCharacterClass(character_class);
    character.SetLevel(1);
    character.SetExperience(0);
    character.SetHealth(100);
    character.SetMana(50);
    character.SetStrength(10);
    character.SetIntelligence(10);
    character.SetAgility(10);
    character.SetStamina(10);
    character.SetMapId(1);
    character.SetPosition(0.0f, 0.0f, 0.0f);
    character.SetCreatedTime(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    character.SetLastLoginTime(character.GetCreatedTime());
    
    characters_[character.GetCharacterId()] = character;
    
    player_characters_[player_id].push_back(character.GetCharacterId());
    
    return true;
}

bool CharacterManager::DeleteCharacter(std::uint64_t character_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    std::uint64_t player_id = it->second.GetPlayerId();
    
    auto player_it = player_characters_.find(player_id);
    if (player_it != player_characters_.end()) {
        auto& character_ids = player_it->second;
        character_ids.erase(std::remove(character_ids.begin(), character_ids.end(), character_id), character_ids.end());
    }
    
    characters_.erase(it);
    
    return true;
}

const Character* CharacterManager::GetCharacter(std::uint64_t character_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it != characters_.end()) {
        return &it->second;
    }
    
    return nullptr;
}

std::vector<Character> CharacterManager::GetPlayerCharacters(std::uint64_t player_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Character> player_character_list;
    
    auto player_it = player_characters_.find(player_id);
    if (player_it != player_characters_.end()) {
        for (std::uint64_t character_id : player_it->second) {
            auto character_it = characters_.find(character_id);
            if (character_it != characters_.end()) {
                player_character_list.push_back(character_it->second);
            }
        }
    }
    
    return player_character_list;
}

bool CharacterManager::UpdateCharacterLevel(std::uint64_t character_id, int level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetLevel(level);
    return true;
}

bool CharacterManager::UpdateCharacterExperience(std::uint64_t character_id, int experience) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetExperience(experience);
    return true;
}

bool CharacterManager::UpdateCharacterHealth(std::uint64_t character_id, int health) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetHealth(health);
    return true;
}

bool CharacterManager::UpdateCharacterMana(std::uint64_t character_id, int mana) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetMana(mana);
    return true;
}

bool CharacterManager::UpdateCharacterAttributes(std::uint64_t character_id, int strength, int intelligence, int agility, int stamina) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetStrength(strength);
    it->second.SetIntelligence(intelligence);
    it->second.SetAgility(agility);
    it->second.SetStamina(stamina);
    return true;
}

bool CharacterManager::UpdateCharacterPosition(std::uint64_t character_id, std::uint32_t map_id, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = characters_.find(character_id);
    if (it == characters_.end()) {
        return false;
    }
    
    it->second.SetMapId(map_id);
    it->second.SetPosition(x, y, z);
    return true;
}

std::uint64_t CharacterManager::GenerateCharacterId() {
    std::lock_guard<std::mutex> lock(mutex_);
    return next_character_id_++;
}

} // namespace role
} // namespace mmo