#include "chat/chat_lua_binding.h"
#include <cstdint>
#include <string>
#include <LuaBridge/LuaBridge.h>

namespace mmo {
namespace chat {

void ChatLuaBinding::Register(LuaEngine& engine) {
    RegisterChannelConstants(engine);
    RegisterChatMessage(engine);
    RegisterChatManager(engine);
}

void ChatLuaBinding::RegisterChannelConstants(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Chat")
            .beginNamespace("Channel")
                .addVariable("WORLD", static_cast<int>(CHANNEL_WORLD))
                .addVariable("PRIVATE", static_cast<int>(CHANNEL_PRIVATE))
                .addVariable("GUILD", static_cast<int>(CHANNEL_GUILD))
                .addVariable("TEAM", static_cast<int>(CHANNEL_TEAM))
                .addVariable("SYSTEM", static_cast<int>(CHANNEL_SYSTEM))
            .endNamespace()
        .endNamespace();
}

void ChatLuaBinding::RegisterChatMessage(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Chat")
            .beginClass<ChatMessageStruct>("ChatMessage")
                .addConstructor<void(*)()>()
                .addProperty("sender_id", 
                    [](const ChatMessageStruct* msg) { return static_cast<std::uint64_t>(msg->sender_id); },
                    [](ChatMessageStruct* msg, std::uint64_t val) { msg->sender_id = val; })
                .addProperty("sender_name", 
                    [](const ChatMessageStruct* msg) { return msg->sender_name; },
                    [](ChatMessageStruct* msg, const std::string& val) { msg->sender_name = val; })
                .addProperty("channel", 
                    [](const ChatMessageStruct* msg) { return msg->channel; },
                    [](ChatMessageStruct* msg, int val) { msg->channel = val; })
                .addProperty("content", 
                    [](const ChatMessageStruct* msg) { return msg->content; },
                    [](ChatMessageStruct* msg, const std::string& val) { msg->content = val; })
                .addProperty("timestamp", 
                    [](const ChatMessageStruct* msg) { return msg->timestamp; },
                    [](ChatMessageStruct* msg, std::int64_t val) { msg->timestamp = val; })
                .addProperty("target_id", 
                    [](const ChatMessageStruct* msg) { return static_cast<std::uint64_t>(msg->target_id); },
                    [](ChatMessageStruct* msg, std::uint64_t val) { msg->target_id = val; })
            .endClass()
        .endNamespace();
}

void ChatLuaBinding::RegisterChatManager(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Chat")
            .beginClass<ChatManager>("ChatManager")
                .addStaticFunction("instance", &ChatManager::Instance)
                .addFunction("initialize", &ChatManager::Initialize)
                .addFunction("shutdown", &ChatManager::Shutdown)
                .addFunction("send_chat", &ChatManager::SendChat)
                .addFunction("send_system_message", &ChatManager::SendSystemMessage)
                .addFunction("send_private_message", &ChatManager::SendPrivateMessage)
                .addFunction("get_chat_history", &ChatManager::GetChatHistory)
                .addFunction("set_player_session", &ChatManager::SetPlayerSession)
                .addFunction("remove_player_session", &ChatManager::RemovePlayerSession)
                .addFunction("get_player_session", &ChatManager::GetPlayerSession)
                .addFunction("is_muted", &ChatManager::IsMuted)
                .addFunction("mute_player", &ChatManager::MutePlayer)
                .addFunction("unmute_player", &ChatManager::UnmutePlayer)
            .endClass()
        .endNamespace();
}

} // namespace chat
} // namespace mmo
