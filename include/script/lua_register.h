#pragma once

#include "script/lua_engine.h"
#include "core/logger.h"
#include "core/config.h"
#include "network/connection.h"
#include "network/tcp_server.h"
#include "service/service_base.h"

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
#ifdef USE_PROTOBUF
        RegisterProtobuf(engine);
#endif
    }

    static void RegisterLogger(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("mmo")
            .endNamespace();
    }

    static void RegisterConfig(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("mmo")
            .endNamespace();
    }

    static void RegisterNetwork(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("mmo")
            .endNamespace();
    }

    static void RegisterUtils(LuaEngine& engine) {
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("mmo")
            .endNamespace();
    }

    static void RegisterService(LuaEngine& engine);

#ifdef USE_PROTOBUF
    static void RegisterProtobuf(LuaEngine& engine) {
        LuaProtobuf::Instance().Initialize(engine.GetState());
        
        luabridge::getGlobalNamespace(engine.GetState())
            .beginNamespace("mmo")
                .beginNamespace("msg")
                    .addVariable("MSG_HEARTBEAT", MSG_HEARTBEAT)
                    .addVariable("MSG_ERROR", MSG_ERROR)
                    .addVariable("MSG_LOGIN_REQUEST", MSG_LOGIN_REQUEST)
                    .addVariable("MSG_LOGIN_RESPONSE", MSG_LOGIN_RESPONSE)
                    .addVariable("MSG_LOGOUT_REQUEST", MSG_LOGOUT_REQUEST)
                    .addVariable("MSG_LOGOUT_RESPONSE", MSG_LOGOUT_RESPONSE)
                    .addVariable("MSG_REGISTER_REQUEST", MSG_REGISTER_REQUEST)
                    .addVariable("MSG_REGISTER_RESPONSE", MSG_REGISTER_RESPONSE)
                    .addVariable("MSG_CREATE_ROLE_REQUEST", MSG_CREATE_ROLE_REQUEST)
                    .addVariable("MSG_CREATE_ROLE_RESPONSE", MSG_CREATE_ROLE_RESPONSE)
                    .addVariable("MSG_PLAYER_MOVE_REQUEST", MSG_PLAYER_MOVE_REQUEST)
                    .addVariable("MSG_PLAYER_MOVE_RESPONSE", MSG_PLAYER_MOVE_RESPONSE)
                    .addVariable("MSG_PLAYER_MOVE_NOTIFY", MSG_PLAYER_MOVE_NOTIFY)
                    .addVariable("MSG_PLAYER_ENTER_SCENE", MSG_PLAYER_ENTER_SCENE)
                    .addVariable("MSG_PLAYER_LEAVE_SCENE", MSG_PLAYER_LEAVE_SCENE)
                    .addVariable("MSG_GET_PLAYER_INFO_REQUEST", MSG_GET_PLAYER_INFO_REQUEST)
                    .addVariable("MSG_GET_PLAYER_INFO_RESPONSE", MSG_GET_PLAYER_INFO_RESPONSE)
                    .addVariable("MSG_UPDATE_PLAYER_INFO", MSG_UPDATE_PLAYER_INFO)
                    .addVariable("MSG_SEND_CHAT_REQUEST", MSG_SEND_CHAT_REQUEST)
                    .addVariable("MSG_SEND_CHAT_RESPONSE", MSG_SEND_CHAT_RESPONSE)
                    .addVariable("MSG_RECEIVE_CHAT", MSG_RECEIVE_CHAT)
                    .addVariable("MSG_GET_CHAT_HISTORY_REQUEST", MSG_GET_CHAT_HISTORY_REQUEST)
                    .addVariable("MSG_GET_CHAT_HISTORY_RESPONSE", MSG_GET_CHAT_HISTORY_RESPONSE)
                .endNamespace()
            .endNamespace();
    }
#endif
};

}
