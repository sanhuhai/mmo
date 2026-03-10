#pragma once

#include "script/lua_engine.h"
#include "item/item_manager.h"

namespace mmo {
namespace item {

// Lua binding class
class ItemLuaBinding {
public:
    static void Register(LuaEngine& engine);
    
private:
    static void RegisterItemTypeConstants(LuaEngine& engine);
    static void RegisterUseResultConstants(LuaEngine& engine);
    static void RegisterItem(LuaEngine& engine);
    static void RegisterPlayerItem(LuaEngine& engine);
    static void RegisterItemManager(LuaEngine& engine);
};

} // namespace item
} // namespace mmo