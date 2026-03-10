#include "character/character_lua_binding.h"
#include "script/lua_engine.h"
#include "character/character_manager.h"

namespace mmo {
namespace character {

void CharacterLuaBinding::Register(LuaEngine& engine) {
    RegisterCharacterClassConstants(engine);
    RegisterCharacter(engine);
    RegisterCharacterManager(engine);
}

void CharacterLuaBinding::RegisterCharacterClassConstants(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Character")
            .beginNamespace("Class")
                .addVariable("WARRIOR", static_cast<int>(CHARACTER_CLASS_WARRIOR))
                .addVariable("MAGE", static_cast<int>(CHARACTER_CLASS_MAGE))
                .addVariable("ARCHER", static_cast<int>(CHARACTER_CLASS_ARCHER))
                .addVariable("ROGUE", static_cast<int>(CHARACTER_CLASS_ROGUE))
                .addVariable("PRIEST", static_cast<int>(CHARACTER_CLASS_PRIEST))
            .endNamespace()
        .endNamespace();
}

void CharacterLuaBinding::RegisterCharacter(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Character")
            .beginClass<Character>("Character")
                .addConstructor<void(*)()>()
                .addProperty("character_id", &Character::GetCharacterId, &Character::SetCharacterId)
                .addProperty("player_id", &Character::GetPlayerId, &Character::SetPlayerId)
                .addProperty("name", &Character::GetName, &Character::SetName)
                .addProperty("character_class", &Character::GetCharacterClass, &Character::SetCharacterClass)
                .addProperty("level", &Character::GetLevel, &Character::SetLevel)
                .addProperty("experience", &Character::GetExperience, &Character::SetExperience)
                .addProperty("health", &Character::GetHealth, &Character::SetHealth)
                .addProperty("mana", &Character::GetMana, &Character::SetMana)
                .addProperty("strength", &Character::GetStrength, &Character::SetStrength)
                .addProperty("intelligence", &Character::GetIntelligence, &Character::SetIntelligence)
                .addProperty("agility", &Character::GetAgility, &Character::SetAgility)
                .addProperty("stamina", &Character::GetStamina, &Character::SetStamina)
                .addProperty("map_id", &Character::GetMapId, &Character::SetMapId)
                .addProperty("position_x", &Character::GetPositionX, &Character::SetPositionX)
                .addProperty("position_y", &Character::GetPositionY, &Character::SetPositionY)
                .addProperty("position_z", &Character::GetPositionZ, &Character::SetPositionZ)
                .addProperty("created_time", &Character::GetCreatedTime, &Character::SetCreatedTime)
                .addProperty("last_login_time", &Character::GetLastLoginTime, &Character::SetLastLoginTime)
            .endClass()
        .endNamespace();
}

void CharacterLuaBinding::RegisterCharacterManager(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Character")
            .beginClass<CharacterManager>("CharacterManager")
                .addStaticFunction("instance", &CharacterManager::Instance)
                .addFunction("initialize", &CharacterManager::Initialize)
                .addFunction("shutdown", &CharacterManager::Shutdown)
                .addFunction("create_character", &CharacterManager::CreateCharacter)
                .addFunction("delete_character", &CharacterManager::DeleteCharacter)
                .addFunction("get_character", &CharacterManager::GetCharacter)
                .addFunction("get_player_characters", &CharacterManager::GetPlayerCharacters)
                .addFunction("update_character_level", &CharacterManager::UpdateCharacterLevel)
                .addFunction("update_character_experience", &CharacterManager::UpdateCharacterExperience)
                .addFunction("update_character_health", &CharacterManager::UpdateCharacterHealth)
                .addFunction("update_character_mana", &CharacterManager::UpdateCharacterMana)
                .addFunction("update_character_attributes", &CharacterManager::UpdateCharacterAttributes)
                .addFunction("update_character_position", &CharacterManager::UpdateCharacterPosition)
                .addFunction("generate_character_id", &CharacterManager::GenerateCharacterId)
            .endClass()
        .endNamespace();
}

} // namespace character
} // namespace mmo