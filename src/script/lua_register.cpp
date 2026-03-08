#include "script/lua_register.h"

// Forward declaration for protobuf Lua registration
extern void RegisterProtoToLua(lua_State* L);

namespace mmo {

void LuaRegister::RegisterService(LuaEngine& engine) {
    // Register protobuf messages to Lua
    #ifdef USE_PROTOBUF
        RegisterProtoToLua(engine.GetState());
    #endif
    
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("mmo")
        .endNamespace();
}

} 
