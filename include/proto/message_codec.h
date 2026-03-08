#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "core/logger.h"
#include "proto/game.pb.h"

namespace mmo {

class MessageCodec {
public:
    using MessagePtr = std::shared_ptr<google::protobuf::Message>;
    using MessageCreator = std::function<MessagePtr()>;

    static MessageCodec& Instance() {
        static MessageCodec instance;
        return instance;
    }

    bool Initialize() {
        RegisterMessage<mmo::Heartbeat>(MSG_HEARTBEAT);
        RegisterMessage<mmo::LoginRequest>(MSG_LOGIN_REQUEST);
        RegisterMessage<mmo::LoginResponse>(MSG_LOGIN_RESPONSE);
        RegisterMessage<mmo::LogoutRequest>(MSG_LOGOUT_REQUEST);
        RegisterMessage<mmo::LogoutResponse>(MSG_LOGOUT_RESPONSE);
        RegisterMessage<mmo::RegisterRequest>(MSG_REGISTER_REQUEST);
        RegisterMessage<mmo::RegisterResponse>(MSG_REGISTER_RESPONSE);
        RegisterMessage<mmo::CreateRoleRequest>(MSG_CREATE_ROLE_REQUEST);
        RegisterMessage<mmo::CreateRoleResponse>(MSG_CREATE_ROLE_RESPONSE);
        RegisterMessage<mmo::PlayerMoveRequest>(MSG_PLAYER_MOVE_REQUEST);
        RegisterMessage<mmo::PlayerMoveResponse>(MSG_PLAYER_MOVE_RESPONSE);
        RegisterMessage<mmo::PlayerMoveNotify>(MSG_PLAYER_MOVE_NOTIFY);
        RegisterMessage<mmo::PlayerEnterSceneNotify>(MSG_PLAYER_ENTER_SCENE);
        RegisterMessage<mmo::PlayerLeaveSceneNotify>(MSG_PLAYER_LEAVE_SCENE);
        RegisterMessage<mmo::GetPlayerInfoRequest>(MSG_GET_PLAYER_INFO_REQUEST);
        RegisterMessage<mmo::GetPlayerInfoResponse>(MSG_GET_PLAYER_INFO_RESPONSE);
        RegisterMessage<mmo::UpdatePlayerInfoNotify>(MSG_UPDATE_PLAYER_INFO);
        RegisterMessage<mmo::SendChatRequest>(MSG_SEND_CHAT_REQUEST);
        RegisterMessage<mmo::SendChatResponse>(MSG_SEND_CHAT_RESPONSE);
        RegisterMessage<mmo::ReceiveChatNotify>(MSG_RECEIVE_CHAT);
        RegisterMessage<mmo::GetChatHistoryRequest>(MSG_GET_CHAT_HISTORY_REQUEST);
        RegisterMessage<mmo::GetChatHistoryResponse>(MSG_GET_CHAT_HISTORY_RESPONSE);
        RegisterMessage<mmo::ErrorResponse>(MSG_ERROR);

        LOG_INFO("MessageCodec initialized with {} message types", message_creators_.size());
        return true;
    }

    template<typename T>
    void RegisterMessage(MessageType type) {
        message_creators_[type] = []() -> MessagePtr {
            return std::make_shared<T>();
        };
        type_to_name_[type] = T::descriptor()->full_name();
    }

    MessagePtr CreateMessage(MessageType type) {
        auto it = message_creators_.find(type);
        if (it != message_creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    MessageType GetMessageType(const std::string& message_name) const {
        for (const auto& pair : type_to_name_) {
            if (pair.second == message_name) {
                return pair.first;
            }
        }
        return MSG_ERROR;
    }

    std::string GetMessageName(MessageType type) const {
        auto it = type_to_name_.find(type);
        if (it != type_to_name_.end()) {
            return it->second;
        }
        return "";
    }

    std::vector<uint8_t> Encode(const google::protobuf::Message& message) {
        std::vector<uint8_t> buffer;
        buffer.resize(message.ByteSizeLong());
        message.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));
        return buffer;
    }

    std::vector<uint8_t> EncodeWithType(const google::protobuf::Message& message) {
        MessageType type = GetMessageType(message.GetDescriptor()->full_name());
        
        std::vector<uint8_t> payload = Encode(message);
        
        GameMessage game_msg;
        game_msg.set_type(type);
        game_msg.set_sequence(next_sequence_++);
        game_msg.set_timestamp(GetCurrentTimestamp());
        game_msg.set_payload(payload.data(), payload.size());
        
        return Encode(game_msg);
    }

    MessagePtr Decode(const std::vector<uint8_t>& data) {
        GameMessage game_msg;
        if (!game_msg.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
            LOG_ERROR("Failed to parse GameMessage");
            return nullptr;
        }

        MessagePtr message = CreateMessage(game_msg.type());
        if (!message) {
            LOG_ERROR("Unknown message type: {}", game_msg.type());
            return nullptr;
        }

        if (!message->ParseFromString(game_msg.payload())) {
            LOG_ERROR("Failed to parse payload for message type: {}", game_msg.type());
            return nullptr;
        }

        return message;
    }

    MessagePtr Decode(const uint8_t* data, size_t size) {
        return Decode(std::vector<uint8_t>(data, data + size));
    }

    template<typename T>
    std::shared_ptr<T> DecodeAs(const std::vector<uint8_t>& data) {
        MessagePtr message = Decode(data);
        if (message) {
            return std::dynamic_pointer_cast<T>(message);
        }
        return nullptr;
    }

    std::vector<uint8_t> PackMessage(const google::protobuf::Message& message, uint64_t sequence = 0) {
        MessageType type = GetMessageType(message.GetDescriptor()->full_name());
        
        std::vector<uint8_t> payload;
        payload.resize(message.ByteSizeLong());
        message.SerializeToArray(payload.data(), static_cast<int>(payload.size()));

        GameMessage game_msg;
        game_msg.set_type(type);
        game_msg.set_sequence(sequence == 0 ? next_sequence_++ : sequence);
        game_msg.set_timestamp(GetCurrentTimestamp());
        game_msg.set_payload(payload.data(), payload.size());

        std::vector<uint8_t> result;
        result.resize(game_msg.ByteSizeLong());
        game_msg.SerializeToArray(result.data(), static_cast<int>(result.size()));

        return result;
    }

    std::vector<uint8_t> PackMessage(MessageType type, const std::vector<uint8_t>& payload, uint64_t sequence = 0) {
        GameMessage game_msg;
        game_msg.set_type(type);
        game_msg.set_sequence(sequence == 0 ? next_sequence_++ : sequence);
        game_msg.set_timestamp(GetCurrentTimestamp());
        game_msg.set_payload(payload.data(), payload.size());

        std::vector<uint8_t> result;
        result.resize(game_msg.ByteSizeLong());
        game_msg.SerializeToArray(result.data(), static_cast<int>(result.size()));

        return result;
    }

private:
    MessageCodec() = default;
    ~MessageCodec() = default;
    MessageCodec(const MessageCodec&) = delete;
    MessageCodec& operator=(const MessageCodec&) = delete;

    int64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::unordered_map<MessageType, MessageCreator> message_creators_;
    std::unordered_map<MessageType, std::string> type_to_name_;
    std::atomic<uint64_t> next_sequence_{1};
};

}
