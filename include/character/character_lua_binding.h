#pragma once

#include "script/lua_engine.h"

namespace mmo {
namespace character {

// Lua binding class
class CharacterLuaBinding {
public:
    static void Register(LuaEngine& engine);
    
private:
    static void RegisterCharacterClassConstants(LuaEngine& engine);
    static void RegisterCharacter(LuaEngine& engine);
    static void RegisterCharacterManager(LuaEngine& engine);
};

} // namespace character
} // namespace mmo