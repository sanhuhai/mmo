#pragma once

#include <string>
#include <memory>
#include <functional>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>

#include <LuaBridge/LuaBridge.h>

#include "core/logger.h"
#include "proto/message_codec.h"

namespace mmo {

class LuaProtobuf {
public:
    using MessagePtr = std::shared_ptr<google::protobuf::Message>;

    static LuaProtobuf& Instance() {
        static LuaProtobuf instance;
        return instance;
    }

    bool Initialize(lua_State* L) {
        lua_state_ = L;
        
        importer_ = std::make_unique<google::protobuf::compiler::Importer>(
            &disk_source_tree_, &error_collector_);
        
        factory_ = std::make_unique<google::protobuf::DynamicMessageFactory>();

        RegisterToLua();

        LOG_INFO("LuaProtobuf initialized");
        return true;
    }

    bool ImportProto(const std::string& filename) {
        const google::protobuf::FileDescriptor* file_desc = importer_->Import(filename);
        if (file_desc == nullptr) {
            LOG_ERROR("Failed to import proto file: {}", filename);
            return false;
        }
        
        LOG_INFO("Imported proto file: {}", filename);
        return true;
    }

    void SetProtoPath(const std::string& path) {
        disk_source_tree_.MapPath("", path);
    }

    MessagePtr CreateMessage(const std::string& message_name) {
        const google::protobuf::Descriptor* descriptor = importer_->pool()->
            FindMessageTypeByName(message_name);
        
        if (descriptor == nullptr) {
            LOG_ERROR("Message type not found: {}", message_name);
            return nullptr;
        }

        const google::protobuf::Message* prototype = factory_->GetPrototype(descriptor);
        if (prototype == nullptr) {
            LOG_ERROR("Failed to get prototype for: {}", message_name);
            return nullptr;
        }

        return MessagePtr(prototype->New());
    }

    std::string SerializeToString(const google::protobuf::Message& message) {
        return message.SerializeAsString();
    }

    MessagePtr ParseFromString(const std::string& message_name, const std::string& data) {
        MessagePtr message = CreateMessage(message_name);
        if (message && message->ParseFromString(data)) {
            return message;
        }
        return nullptr;
    }

    luabridge::LuaRef MessageToLuaTable(const google::protobuf::Message& message);
    bool LuaTableToMessage(luabridge::LuaRef table, google::protobuf::Message* message);

private:
    LuaProtobuf() = default;
    ~LuaProtobuf() = default;
    LuaProtobuf(const LuaProtobuf&) = delete;
    LuaProtobuf& operator=(const LuaProtobuf&) = delete;

    void RegisterToLua();

    class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
    public:
        void AddError(const std::string& filename, int line, int column,
                      const std::string& message) override {
            LOG_ERROR("Proto error in {}:{}:{} - {}", filename, line, column, message);
        }

        void AddWarning(const std::string& filename, int line, int column,
                        const std::string& message) override {
            LOG_WARN("Proto warning in {}:{}:{} - {}", filename, line, column, message);
        }
    };

    lua_State* lua_state_ = nullptr;
    google::protobuf::compiler::DiskSourceTree disk_source_tree_;
    ErrorCollector error_collector_;
    std::unique_ptr<google::protobuf::compiler::Importer> importer_;
    std::unique_ptr<google::protobuf::DynamicMessageFactory> factory_;
};

}
