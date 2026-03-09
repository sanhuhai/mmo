#pragma once

#ifdef USE_RAPIDJSON

#include "core/logger.h"
#include <string>
#include <map>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace mmo {

class JsonUtils {
public:
    static std::string ParseJson(const std::string& json_str) {
        rapidjson::Document doc;
        doc.Parse(json_str.c_str());
        
        if (doc.HasParseError()) {
            LOG_ERROR("JSON parse error: {}", doc.GetParseError());
            return "";
        }
        
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }
    
    static std::string CreateJson(const std::map<std::string, std::string>& data) {
        rapidjson::Document doc;
        doc.SetObject();
        
        for (const auto& pair : data) {
            rapidjson::Value key(pair.first.c_str(), doc.GetAllocator());
            rapidjson::Value value(pair.second.c_str(), doc.GetAllocator());
            doc.AddMember(key, value, doc.GetAllocator());
        }
        
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }
    
    static bool IsValidJson(const std::string& json_str) {
        rapidjson::Document doc;
        doc.Parse(json_str.c_str());
        return !doc.HasParseError();
    }
};

}

#endif // USE_RAPIDJSON