#pragma once

#include "script/lua_engine.h"
#include "core/logger.h"
#include "core/config.h"
#include "network/connection.h"
#include "network/tcp_server.h"
#include "service/service_base.h"
#include "chat/chat_lua_binding.h"
#include "item/item_lua_binding.h"
#include "character/character_lua_binding.h"

#ifdef USE_PROTOBUF
#include "proto/message_codec.h"
#include "proto/lua_protobuf.h"
#endif

namespace mmo {

class LuaRegister {
public:
    static void RegisterAll(LuaEngine& engine) {
        RegisterLogger(engine);
        RegisterConfig(engine);
        RegisterNetwork(engine);
        RegisterUtils(engine);
        RegisterChat(engine);
        RegisterItem(engine);
        RegisterCharacter(engine);
#ifdef USE_PROTOBUF
        RegisterProtobuf(engine);
#endif
    }

    static void RegisterLogger(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("Log")
                .addFunction("debug", [](const std::string& msg) {
                    LOG_DEBUG("%s", msg.c_str());
                })
                .addFunction("info", [](const std::string& msg) {
                    LOG_INFO("%s", msg.c_str());
                })
                .addFunction("warn", [](const std::string& msg) {
                    LOG_WARN("%s", msg.c_str());
                })
                .addFunction("error", [](const std::string& msg) {
                    LOG_ERROR("%s", msg.c_str());
                })
            .endNamespace();
    }

    static void RegisterConfig(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("Config")
                .addFunction("get", [](const std::string& key, const std::string& default_val) {
                    return Config::Instance().Get(key, default_val).AsString();
                })
                .addFunction("get_int", [](const std::string& key, int default_val) {
                    return Config::Instance().Get(key, std::to_string(default_val)).AsInt();
                })
                .addFunction("get_bool", [](const std::string& key, bool default_val) {
                    return Config::Instance().Get(key, std::string(default_val ? "true" : "false")).AsBool();
                })
            .endNamespace();
    }

    static void RegisterNetwork(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("Network")
            .endNamespace();
    }

    static void RegisterUtils(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("Utils")
                .addFunction("time_ms", []() {
                    auto now = std::chrono::system_clock::now();
                    auto duration = now.time_since_epoch();
                    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                })
                .addFunction("time_s", []() {
                    auto now = std::chrono::system_clock::now();
                    auto duration = now.time_since_epoch();
                    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
                })
                .addFunction("sleep", [](int ms) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                })
            .endNamespace();
    }

    static void RegisterService(LuaEngine& engine);

    static void RegisterChat(LuaEngine& engine) {
        chat::ChatLuaBinding::Register(engine);
    }

    static void RegisterItem(LuaEngine& engine) {
        item::ItemLuaBinding::Register(engine);
    }

    static void RegisterCharacter(LuaEngine& engine) {
        character::CharacterLuaBinding::Register(engine);
    }

#ifdef USE_PROTOBUF
    static void RegisterProtobuf(LuaEngine& engine) {
        LuaProtobuf::Instance().Initialize(engine.GetState());
        
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("Msg")
                .addVariable("HEARTBEAT", MSG_HEARTBEAT)
                .addVariable("ERROR", MSG_ERROR)
                .addVariable("LOGIN_REQUEST", MSG_LOGIN_REQUEST)
                .addVariable("LOGIN_RESPONSE", MSG_LOGIN_RESPONSE)
                .addVariable("LOGOUT_REQUEST", MSG_LOGOUT_REQUEST)
                .addVariable("LOGOUT_RESPONSE", MSG_LOGOUT_RESPONSE)
                .addVariable("REGISTER_REQUEST", MSG_REGISTER_REQUEST)
                .addVariable("REGISTER_RESPONSE", MSG_REGISTER_RESPONSE)
                .addVariable("CREATE_ROLE_REQUEST", MSG_CREATE_ROLE_REQUEST)
                .addVariable("CREATE_ROLE_RESPONSE", MSG_CREATE_ROLE_RESPONSE)
                .addVariable("PLAYER_MOVE_REQUEST", MSG_PLAYER_MOVE_REQUEST)
                .addVariable("PLAYER_MOVE_RESPONSE", MSG_PLAYER_MOVE_RESPONSE)
                .addVariable("PLAYER_MOVE_NOTIFY", MSG_PLAYER_MOVE_NOTIFY)
                .addVariable("PLAYER_ENTER_SCENE", MSG_PLAYER_ENTER_SCENE)
                .addVariable("PLAYER_LEAVE_SCENE", MSG_PLAYER_LEAVE_SCENE)
                .addVariable("GET_PLAYER_INFO_REQUEST", MSG_GET_PLAYER_INFO_REQUEST)
                .addVariable("GET_PLAYER_INFO_RESPONSE", MSG_GET_PLAYER_INFO_RESPONSE)
                .addVariable("UPDATE_PLAYER_INFO", MSG_UPDATE_PLAYER_INFO)
                .addVariable("SEND_CHAT_REQUEST", MSG_SEND_CHAT_REQUEST)
                .addVariable("SEND_CHAT_RESPONSE", MSG_SEND_CHAT_RESPONSE)
                .addVariable("RECEIVE_CHAT", MSG_RECEIVE_CHAT)
                .addVariable("GET_CHAT_HISTORY_REQUEST", MSG_GET_CHAT_HISTORY_REQUEST)
                .addVariable("GET_CHAT_HISTORY_RESPONSE", MSG_GET_CHAT_HISTORY_RESPONSE)
            .endNamespace();
    }
#endif
};

}