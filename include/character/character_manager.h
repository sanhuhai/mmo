#pragma once

#include "character.h"
#include <vector>
#include <map>
#include <mutex>

namespace mmo {
namespace character {

// Character manager - singleton pattern
class CharacterManager {
public:
    static CharacterManager& Instance();
    
    // Initialize/Shutdown
    bool Initialize();
    void Shutdown();
    
    // Character management
    bool CreateCharacter(std::uint64_t player_id, const std::string& name, int character_class);
    bool DeleteCharacter(std::uint64_t character_id);
    const Character* GetCharacter(std::uint64_t character_id) const;
    std::vector<Character> GetPlayerCharacters(std::uint64_t player_id) const;
    
    // Character attributes
    bool UpdateCharacterLevel(std::uint64_t character_id, int level);
    bool UpdateCharacterExperience(std::uint64_t character_id, int experience);
    bool UpdateCharacterHealth(std::uint64_t character_id, int health);
    bool UpdateCharacterMana(std::uint64_t character_id, int mana);
    bool UpdateCharacterAttributes(std::uint64_t character_id, int strength, int intelligence, int agility, int stamina);
    bool UpdateCharacterPosition(std::uint64_t character_id, std::uint32_t map_id, float x, float y, float z);
    
    // Generate character ID
    std::uint64_t GenerateCharacterId();
    
private:
    CharacterManager();
    ~CharacterManager();
    CharacterManager(const CharacterManager&) = delete;
    CharacterManager& operator=(const CharacterManager&) = delete;
    
    bool initialized_;
    mutable std::mutex mutex_;
    
    // Characters: character_id -> Character
    std::map<std::uint64_t, Character> characters_;
    
    // Player characters: player_id -> vector<character_id>
    std::map<std::uint64_t, std::vector<std::uint64_t>> player_characters_;
    
    // Character ID generator
    std::uint64_t next_character_id_;
};

} // namespace character
} // namespace mmo