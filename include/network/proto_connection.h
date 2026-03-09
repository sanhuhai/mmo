#pragma once

#include "network/connection.h"
#include "proto/message_codec.h"
#include <memory>
#include <functional>

namespace mmo {

class ProtoConnection : public std::enable_shared_from_this<ProtoConnection> {
public:
    using Ptr = std::shared_ptr<ProtoConnection>;
    using MessageHandler = std::function<void(Ptr, MessageType, MessageCodec::MessagePtr)>;

    explicit ProtoConnection(Connection::Ptr connection)
        : connection_(connection) {
    }

    ~ProtoConnection() = default;

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

    void Start() {
        if (!connection_) return;

        connection_->SetMessageHandler([this](Connection::Ptr, const std::vector<uint8_t>& data) {
            HandleRawMessage(data);
        });
    }

    void Send(MessageType type, const google::protobuf::Message& message) {
        if (!connection_ || !connection_->IsConnected()) {
            return;
        }

        std::vector<uint8_t> data = MessageCodec::Instance().PackMessage(message);
        connection_->Send(data);
    }

    void SendRaw(const std::vector<uint8_t>& data) {
        if (connection_ && connection_->IsConnected()) {
            connection_->Send(data);
        }
    }

    void SendError(int32_t error_code, const std::string& error_message) {
        ErrorResponse error;
        error.set_error_code(error_code);
        error.set_error_message(error_message);
        Send(MSG_ERROR, error);
    }

    uint32_t GetConnectionId() const {
        return connection_ ? connection_->GetConnectionId() : 0;
    }

    std::string GetRemoteAddress() const {
        return connection_ ? connection_->GetRemoteAddress() : "";
    }

    bool IsConnected() const {
        return connection_ && connection_->IsConnected();
    }

    void Close() {
        if (connection_) {
            connection_->Close();
        }
    }

    Connection::Ptr GetBaseConnection() {
        return connection_;
    }

private:
    void HandleRawMessage(const std::vector<uint8_t>& data) {
        GameMessage game_msg;
        if (!game_msg.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
            LOG_ERROR("Failed to parse GameMessage from connection {}", GetConnectionId());
            return;
        }

        MessageCodec::MessagePtr message = MessageCodec::Instance().CreateMessage(game_msg.type());
        if (!message) {
            LOG_ERROR("Unknown message type: {} from connection {}", 
                      static_cast<int>(game_msg.type()), GetConnectionId());
            SendError(1, "Unknown message type");
            return;
        }

        if (!message->ParseFromString(game_msg.payload())) {
            LOG_ERROR("Failed to parse payload for message type: {}", 
                      static_cast<int>(game_msg.type()));
            SendError(2, "Failed to parse message");
            return;
        }

        if (message_handler_) {
            message_handler_(shared_from_this(), game_msg.type(), message);
        }
    }

    Connection::Ptr connection_;
    MessageHandler message_handler_;
};

}
