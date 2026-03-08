#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"

// Include proto headers
#include "common.pb.h"
#include "game.pb.h"

namespace mmo {

// Register protobuf messages to Lua
void RegisterProtoToLua(lua_State* L) {
    using namespace luabridge;
    
    // Register common types
    getGlobalNamespace(L)
        .beginNamespace("mmo")
            // Register MessageType enum
            .addVariable("MSG_HEARTBEAT", mmo::MessageType::MSG_HEARTBEAT)
            .addVariable("MSG_ERROR", mmo::MessageType::MSG_ERROR)
            .addVariable("MSG_LOGIN_REQUEST", mmo::MessageType::MSG_LOGIN_REQUEST)
            .addVariable("MSG_LOGIN_RESPONSE", mmo::MessageType::MSG_LOGIN_RESPONSE)
            .addVariable("MSG_LOGOUT_REQUEST", mmo::MessageType::MSG_LOGOUT_REQUEST)
            .addVariable("MSG_LOGOUT_RESPONSE", mmo::MessageType::MSG_LOGOUT_RESPONSE)
            .addVariable("MSG_REGISTER_REQUEST", mmo::MessageType::MSG_REGISTER_REQUEST)
            .addVariable("MSG_REGISTER_RESPONSE", mmo::MessageType::MSG_REGISTER_RESPONSE)
            .addVariable("MSG_CREATE_ROLE_REQUEST", mmo::MessageType::MSG_CREATE_ROLE_REQUEST)
            .addVariable("MSG_CREATE_ROLE_RESPONSE", mmo::MessageType::MSG_CREATE_ROLE_RESPONSE)
            .addVariable("MSG_PLAYER_MOVE_REQUEST", mmo::MessageType::MSG_PLAYER_MOVE_REQUEST)
            .addVariable("MSG_PLAYER_MOVE_RESPONSE", mmo::MessageType::MSG_PLAYER_MOVE_RESPONSE)
            .addVariable("MSG_PLAYER_MOVE_NOTIFY", mmo::MessageType::MSG_PLAYER_MOVE_NOTIFY)
            .addVariable("MSG_PLAYER_ENTER_SCENE", mmo::MessageType::MSG_PLAYER_ENTER_SCENE)
            .addVariable("MSG_PLAYER_LEAVE_SCENE", mmo::MessageType::MSG_PLAYER_LEAVE_SCENE)
            .addVariable("MSG_GET_PLAYER_INFO_REQUEST", mmo::MessageType::MSG_GET_PLAYER_INFO_REQUEST)
            .addVariable("MSG_GET_PLAYER_INFO_RESPONSE", mmo::MessageType::MSG_GET_PLAYER_INFO_RESPONSE)
            .addVariable("MSG_UPDATE_PLAYER_INFO", mmo::MessageType::MSG_UPDATE_PLAYER_INFO)
            .addVariable("MSG_SEND_CHAT_REQUEST", mmo::MessageType::MSG_SEND_CHAT_REQUEST)
            .addVariable("MSG_SEND_CHAT_RESPONSE", mmo::MessageType::MSG_SEND_CHAT_RESPONSE)
            .addVariable("MSG_RECEIVE_CHAT", mmo::MessageType::MSG_RECEIVE_CHAT)
            .addVariable("MSG_GET_CHAT_HISTORY_REQUEST", mmo::MessageType::MSG_GET_CHAT_HISTORY_REQUEST)
            .addVariable("MSG_GET_CHAT_HISTORY_RESPONSE", mmo::MessageType::MSG_GET_CHAT_HISTORY_RESPONSE)
            
            // Register Vector3
            .beginClass<Vector3>("Vector3")
                .addConstructor<void(*)()>()
                .addProperty("x", &Vector3::x, &Vector3::set_x)
                .addProperty("y", &Vector3::y, &Vector3::set_y)
                .addProperty("z", &Vector3::z, &Vector3::set_z)
            .endClass()
            
            // Register Heartbeat
            .beginClass<Heartbeat>("Heartbeat")
                .addConstructor<void(*)()>()
                .addProperty("timestamp", &Heartbeat::timestamp, &Heartbeat::set_timestamp)
            .endClass()
            
            // Register GameMessage
            .beginClass<GameMessage>("GameMessage")
                .addConstructor<void(*)()>()
                .addProperty("type", &GameMessage::type, &GameMessage::set_type)
                .addProperty("sequence", &GameMessage::sequence, &GameMessage::set_sequence)
                .addProperty("timestamp", &GameMessage::timestamp, &GameMessage::set_timestamp)
            .endClass()
            
            // Register ErrorResponse
            .beginClass<ErrorResponse>("ErrorResponse")
                .addConstructor<void(*)()>()
                .addProperty("error_code", &ErrorResponse::error_code, &ErrorResponse::set_error_code)
                .addProperty("error_message", &ErrorResponse::error_message)
            .endClass()
        .endNamespace();
}

} // namespace mmo
