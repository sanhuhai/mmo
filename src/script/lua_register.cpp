#include "script/lua_register.h"

namespace mmo {

void LuaRegister::RegisterService(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("mmo")
        .endNamespace();
}

}
