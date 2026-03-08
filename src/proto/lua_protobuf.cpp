#include "proto/lua_protobuf.h"

namespace mmo {

luabridge::LuaRef LuaProtobuf::MessageToLuaTable(const google::protobuf::Message& message) {
    luabridge::LuaRef table = luabridge::newTable(lua_state_);
    
    const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
    const google::protobuf::Reflection* reflection = message.GetReflection();
    
    for (int i = 0; i < descriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* field = descriptor->field(i);
        const std::string& field_name = field->name();
        
        if (field->is_repeated()) {
            luabridge::LuaRef array = luabridge::newTable(lua_state_);
            int size = reflection->FieldSize(message, field);
            for (int j = 0; j < size; ++j) {
                switch (field->cpp_type()) {
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        array[j + 1] = reflection->GetRepeatedInt32(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        array[j + 1] = reflection->GetRepeatedInt64(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        array[j + 1] = reflection->GetRepeatedUInt32(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        array[j + 1] = reflection->GetRepeatedUInt64(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        array[j + 1] = reflection->GetRepeatedDouble(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        array[j + 1] = reflection->GetRepeatedFloat(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        array[j + 1] = reflection->GetRepeatedBool(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                        array[j + 1] = reflection->GetRepeatedEnumValue(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        array[j + 1] = reflection->GetRepeatedString(message, field, j);
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                        array[j + 1] = MessageToLuaTable(reflection->GetRepeatedMessage(message, field, j));
                        break;
                    default:
                        break;
                }
            }
            table[field_name] = array;
        } else {
            if (!reflection->HasField(message, field) && !field->is_required()) {
                continue;
            }
            
            switch (field->cpp_type()) {
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    table[field_name] = reflection->GetInt32(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    table[field_name] = reflection->GetInt64(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    table[field_name] = reflection->GetUInt32(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    table[field_name] = reflection->GetUInt64(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    table[field_name] = reflection->GetDouble(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    table[field_name] = reflection->GetFloat(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    table[field_name] = reflection->GetBool(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    table[field_name] = reflection->GetEnumValue(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    table[field_name] = reflection->GetString(message, field);
                    break;
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    table[field_name] = MessageToLuaTable(reflection->GetMessage(message, field));
                    break;
                default:
                    break;
            }
        }
    }
    
    return table;
}

bool LuaProtobuf::LuaTableToMessage(luabridge::LuaRef table, google::protobuf::Message* message) {
    if (!table.isTable()) {
        return false;
    }
    
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();
    
    for (int i = 0; i < descriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* field = descriptor->field(i);
        const std::string& field_name = field->name();
        
        if (!table[field_name].isNil()) {
            luabridge::LuaRef value = table[field_name];
            
            if (field->is_repeated()) {
                if (!value.isTable()) {
                    continue;
                }
                
                for (int j = 1; j <= value.length(); ++j) {
                    luabridge::LuaRef item = value[j];
                    
                    switch (field->cpp_type()) {
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                            reflection->AddInt32(message, field, item.cast<int32_t>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                            reflection->AddInt64(message, field, item.cast<int64_t>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                            reflection->AddUInt32(message, field, item.cast<uint32_t>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                            reflection->AddUInt64(message, field, item.cast<uint64_t>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                            reflection->AddDouble(message, field, item.cast<double>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                            reflection->AddFloat(message, field, item.cast<float>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                            reflection->AddBool(message, field, item.cast<bool>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                            reflection->AddEnumValue(message, field, item.cast<int>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                            reflection->AddString(message, field, item.cast<std::string>());
                            break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                            google::protobuf::Message* sub_message = reflection->AddMessage(message, field);
                            LuaTableToMessage(item, sub_message);
                            break;
                        }
                        default:
                            break;
                    }
                }
            } else {
                switch (field->cpp_type()) {
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        reflection->SetInt32(message, field, value.cast<int32_t>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        reflection->SetInt64(message, field, value.cast<int64_t>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        reflection->SetUInt32(message, field, value.cast<uint32_t>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        reflection->SetUInt64(message, field, value.cast<uint64_t>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        reflection->SetDouble(message, field, value.cast<double>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        reflection->SetFloat(message, field, value.cast<float>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        reflection->SetBool(message, field, value.cast<bool>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                        reflection->SetEnumValue(message, field, value.cast<int>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        reflection->SetString(message, field, value.cast<std::string>());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                        google::protobuf::Message* sub_message = reflection->MutableMessage(message, field);
                        LuaTableToMessage(value, sub_message);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    
    return true;
}

void LuaProtobuf::RegisterToLua() {
    auto* self = this;

    luabridge::getGlobalNamespace(lua_state_)
        .beginNamespace("protobuf")
            .addFunction("import", [self](const std::string& filename) -> bool {
                return self->ImportProto(filename);
            })
            .addFunction("set_path", [self](const std::string& path) {
                self->SetProtoPath(path);
            })
            .addFunction("create_message", [self](const std::string& name) -> std::string {
                auto message = self->CreateMessage(name);
                if (message) {
                    return self->SerializeToString(*message);
                }
                return "";
            })
            .addFunction("encode", [self](const std::string& message_name, luabridge::LuaRef table) -> std::string {
                auto message = self->CreateMessage(message_name);
                if (message && self->LuaTableToMessage(table, message.get())) {
                    return self->SerializeToString(*message);
                }
                return "";
            })
            .addFunction("decode", [self](const std::string& message_name, const std::string& data) -> luabridge::LuaRef {
                auto message = self->ParseFromString(message_name, data);
                if (message) {
                    return self->MessageToLuaTable(*message);
                }
                return luabridge::LuaRef(lua_state_);
            })
            .addFunction("message_to_table", [self](const std::string& message_name, const std::string& data) -> luabridge::LuaRef {
                auto message = self->ParseFromString(message_name, data);
                if (message) {
                    return self->MessageToLuaTable(*message);
                }
                return luabridge::LuaRef(lua_state_);
            })
            .addFunction("table_to_message", [self](const std::string& message_name, luabridge::LuaRef table) -> std::string {
                auto message = self->CreateMessage(message_name);
                if (message && self->LuaTableToMessage(table, message.get())) {
                    return self->SerializeToString(*message);
                }
                return "";
            })
        .endNamespace();
}

}
