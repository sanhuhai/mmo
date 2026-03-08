#include "script/lua_engine.h"

#include <filesystem>
#include <sstream>

namespace mmo {

LuaEngine::LuaEngine()
    : lua_state_(nullptr) {
}

LuaEngine::~LuaEngine() {
    Shutdown();
}

bool LuaEngine::Initialize() {
    if (lua_state_) {
        return true;
    }

    lua_state_ = luaL_newstate();
    if (!lua_state_) {
        LOG_ERROR("Failed to create Lua state");
        return false;
    }

    lua_atpanic(lua_state_, PanicHandler);

    luaL_openlibs(lua_state_);

    SetupPackagePath();
    RegisterStandardLibs();
    RegisterCustomLibs();

    LOG_INFO("Lua engine initialized (Lua {})", LUA_VERSION);
    return true;
}

void LuaEngine::Shutdown() {
    if (lua_state_) {
        lua_close(lua_state_);
        lua_state_ = nullptr;
        LOG_INFO("Lua engine shutdown");
    }
}

bool LuaEngine::DoFile(const std::string& filename) {
    if (!lua_state_) {
        last_error_ = "Lua state not initialized";
        return false;
    }

    int result = luaL_dofile(lua_state_, filename.c_str());
    if (result != LUA_OK) {
        last_error_ = lua_tostring(lua_state_, -1);
        LOG_ERROR("Lua dofile error: {}", last_error_);
        lua_pop(lua_state_, 1);
        return false;
    }

    return true;
}

bool LuaEngine::DoString(const std::string& code) {
    if (!lua_state_) {
        last_error_ = "Lua state not initialized";
        return false;
    }

    int result = luaL_dostring(lua_state_, code.c_str());
    if (result != LUA_OK) {
        last_error_ = lua_tostring(lua_state_, -1);
        LOG_ERROR("Lua dostring error: {}", last_error_);
        lua_pop(lua_state_, 1);
        return false;
    }

    return true;
}

bool LuaEngine::CallFunction(const std::string& name) {
    return Call(name);
}

bool LuaEngine::CallFunction(const std::string& name, const std::vector<luabridge::LuaRef>& args) {
    try {
        luabridge::LuaRef func = luabridge::getGlobal(lua_state_, name.c_str());
        if (func.isFunction()) {
            if (args.empty()) {
                luabridge::LuaResult result = func();
                return result.wasOk();
            } else {
                return false;
            }
        }
    } catch (const luabridge::LuaException& e) {
        LOG_ERROR("Lua call error: {}", e.what());
    }
    return false;
}

luabridge::LuaRef LuaEngine::GetGlobal(const std::string& name) {
    return luabridge::getGlobal(lua_state_, name.c_str());
}

void LuaEngine::SetGlobal(const std::string& name, const luabridge::LuaRef& value) {
    value.push(lua_state_);
    lua_setglobal(lua_state_, name.c_str());
}

void LuaEngine::AddSearchPath(const std::string& path) {
    lua_getglobal(lua_state_, "package");
    if (lua_istable(lua_state_, -1)) {
        lua_getfield(lua_state_, -1, "path");
        const char* current_path = lua_tostring(lua_state_, -1);
        
        std::string new_path = path;
        if (!new_path.empty() && new_path.back() != '/') {
            new_path += '/';
        }
        new_path += "?.lua";
        
        if (current_path && strlen(current_path) > 0) {
            new_path += ";" + std::string(current_path);
        }
        
        lua_pop(lua_state_, 1);
        lua_pushstring(lua_state_, new_path.c_str());
        lua_setfield(lua_state_, -2, "path");
    }
    lua_pop(lua_state_, 1);
}

void LuaEngine::SetScriptPath(const std::string& path) {
    script_path_ = path;
    AddSearchPath(path);
}

bool LuaEngine::LoadModule(const std::string& module_name) {
    std::string filename = script_path_ + "/" + module_name + ".lua";
    
    if (!std::filesystem::exists(filename)) {
        filename = module_name;
    }
    
    if (DoFile(filename)) {
        loaded_modules_[module_name] = filename;
        return true;
    }
    
    return false;
}

void LuaEngine::ReloadModule(const std::string& module_name) {
    auto it = loaded_modules_.find(module_name);
    if (it != loaded_modules_.end()) {
        DoFile(it->second);
    }
}

void LuaEngine::RegisterGlobalFunction(const std::string& name, lua_CFunction func) {
    lua_register(lua_state_, name.c_str(), func);
}

int LuaEngine::PanicHandler(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    LOG_FATAL("Lua panic: {}", msg ? msg : "unknown error");
    return 0;
}

void* LuaEngine::Alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize;
    
    if (nsize == 0) {
        free(ptr);
        return nullptr;
    }
    
    return realloc(ptr, nsize);
}

void LuaEngine::SetupPackagePath() {
    lua_getglobal(lua_state_, "package");
    if (lua_istable(lua_state_, -1)) {
        std::string path = "./lua/?.lua;./lua/?/init.lua;./?.lua";
        lua_pushstring(lua_state_, path.c_str());
        lua_setfield(lua_state_, -2, "path");
        
#ifdef _WIN32
        std::string cpath = "./?.dll;./?.so";
#else
        std::string cpath = "./?.so;./?.dylib";
#endif
        lua_pushstring(lua_state_, cpath.c_str());
        lua_setfield(lua_state_, -2, "cpath");
    }
    lua_pop(lua_state_, 1);
}

void LuaEngine::RegisterStandardLibs() {
}

void LuaEngine::RegisterCustomLibs() {
    luabridge::getGlobalNamespace(lua_state_)
        .beginNamespace("mmo")
        .endNamespace();
}

}
