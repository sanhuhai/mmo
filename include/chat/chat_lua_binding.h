#pragma once

#include "script/lua_engine.h"
#include "chat/chat_manager.h"

namespace mmo {
namespace chat {

// Lua binding class
class ChatLuaBinding {
public:
    static void Register(LuaEngine& engine);
    
private:
    static void RegisterChannelConstants(LuaEngine& engine);
    static void RegisterChatMessage(LuaEngine& engine);
    static void RegisterChatManager(LuaEngine& engine);
};

} // namespace chat
} // namespace mmo
