#pragma once

#include "script/lua_engine.h"
#include "role/character_manager.h"

namespace mmo {
namespace role {

// Lua binding class
class CharacterLuaBinding {
public:
    static void Register(LuaEngine& engine);
    
private:
    static void RegisterCharacterClassConstants(LuaEngine& engine);
    static void RegisterCharacter(LuaEngine& engine);
    static void RegisterCharacterManager(LuaEngine& engine);
};

} // namespace role
} // namespace mmo