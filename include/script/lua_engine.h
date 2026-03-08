#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "LuaBridge/LuaBridge.h"

#include "core/logger.h"

namespace mmo {

class LuaEngine {
public:
    using Ptr = std::shared_ptr<LuaEngine>;
    using LuaFunction = std::function<int(lua_State*)>;

    LuaEngine();
    ~LuaEngine();

    bool Initialize();
    void Shutdown();

    bool DoFile(const std::string& filename);
    bool DoString(const std::string& code);

    bool CallFunction(const std::string& name);
    bool CallFunction(const std::string& name, const std::vector<luabridge::LuaRef>& args);

    template<typename... Args>
    bool Call(const std::string& name, Args&&... args) {
        try {
            luabridge::LuaRef func = luabridge::getGlobal(lua_state_, name.c_str());
            if (func.isFunction()) {
                luabridge::LuaResult result = func(std::forward<Args>(args)...);
                return result.wasOk();
            }
        } catch (const luabridge::LuaException& e) {
            LOG_ERROR("Lua call error: {}", e.what());
        }
        return false;
    }

    template<typename Ret, typename... Args>
    Ret CallWithReturn(const std::string& name, Args&&... args) {
        try {
            luabridge::LuaRef func = luabridge::getGlobal(lua_state_, name.c_str());
            if (func.isFunction()) {
                luabridge::LuaResult result = func(std::forward<Args>(args)...);
                if (result.wasOk() && result.size() > 0) {
                    return result[0].cast<Ret>().value();
                }
            }
        } catch (const luabridge::LuaException& e) {
            LOG_ERROR("Lua call error: {}", e.what());
        }
        return Ret();
    }

    luabridge::LuaRef GetGlobal(const std::string& name);
    void SetGlobal(const std::string& name, const luabridge::LuaRef& value);

    template<typename T>
    void SetGlobal(const std::string& name, T value) {
        luabridge::setGlobal(lua_state_, value, name.c_str());
    }

    lua_State* GetState() { return lua_state_; }

    void AddSearchPath(const std::string& path);
    void SetScriptPath(const std::string& path);

    bool LoadModule(const std::string& module_name);
    void ReloadModule(const std::string& module_name);

    void RegisterGlobalFunction(const std::string& name, lua_CFunction func);
    
    template<typename Func>
    void RegisterFunction(const std::string& name, Func func) {
        luabridge::getGlobalNamespace(lua_state_)
            .addFunction(name.c_str(), func);
    }

    std::string GetLastError() const { return last_error_; }

    bool HasError() const { return !last_error_.empty(); }

    void ClearError() { last_error_.clear(); }

private:
    static int PanicHandler(lua_State* L);
    static void* Alloc(void* ud, void* ptr, size_t osize, size_t nsize);

    void SetupPackagePath();
    void RegisterStandardLibs();
    void RegisterCustomLibs();

    lua_State* lua_state_;
    std::string script_path_;
    std::string last_error_;
    std::unordered_map<std::string, std::string> loaded_modules_;
};

}
