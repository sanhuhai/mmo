#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace mmo {

class ConfigValue {
public:
    ConfigValue() = default;
    ConfigValue(const std::string& value) : value_(value) {}

    const std::string& AsString() const { return value_; }
    
    int AsInt() const { return std::stoi(value_); }
    
    int64_t AsInt64() const { return std::stoll(value_); }
    
    double AsDouble() const { return std::stod(value_); }
    
    bool AsBool() const {
        std::string lower = value_;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
    }

    std::vector<std::string> AsArray(const char delimiter = ',') const {
        std::vector<std::string> result;
        std::istringstream iss(value_);
        std::string item;
        while (std::getline(iss, item, delimiter)) {
            size_t start = item.find_first_not_of(" \t");
            size_t end = item.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                result.push_back(item.substr(start, end - start + 1));
            }
        }
        return result;
    }

    bool IsEmpty() const { return value_.empty(); }

private:
    std::string value_;
};

class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    bool Load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string line;
        std::string current_section;
        
        while (std::getline(file, line)) {
            line = Trim(line);
            
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                current_section = line.substr(1, line.length() - 2);
                current_section = Trim(current_section);
                continue;
            }

            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = Trim(line.substr(0, pos));
                std::string value = Trim(line.substr(pos + 1));
                
                if (!current_section.empty()) {
                    key = current_section + "." + key;
                }
                
                values_[key] = ConfigValue(value);
            }
        }

        filename_ = filename;
        return true;
    }

    bool Save(const std::string& filename = "") {
        std::string target_file = filename.empty() ? filename_ : filename;
        if (target_file.empty()) {
            return false;
        }

        std::ofstream file(target_file);
        if (!file.is_open()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        std::map<std::string, std::map<std::string, std::string>> sections;
        std::map<std::string, std::string> root_values;
        
        for (const auto& pair : values_) {
            size_t dot_pos = pair.first.find('.');
            if (dot_pos != std::string::npos) {
                std::string section = pair.first.substr(0, dot_pos);
                std::string key = pair.first.substr(dot_pos + 1);
                sections[section][key] = pair.second.AsString();
            } else {
                root_values[pair.first] = pair.second.AsString();
            }
        }

        for (const auto& pair : root_values) {
            file << pair.first << " = " << pair.second << "\n";
        }

        for (const auto& section_pair : sections) {
            file << "\n[" << section_pair.first << "]\n";
            for (const auto& key_pair : section_pair.second) {
                file << key_pair.first << " = " << key_pair.second << "\n";
            }
        }

        return true;
    }

    ConfigValue Get(const std::string& key, const ConfigValue& default_value = ConfigValue()) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = values_.find(key);
        if (it != values_.end()) {
            return it->second;
        }
        return default_value;
    }

    void Set(const std::string& key, const ConfigValue& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[key] = value;
    }

    bool Has(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.find(key) != values_.end();
    }

    void Remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.erase(key);
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.clear();
    }

    std::vector<std::string> GetKeys() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        for (const auto& pair : values_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::map<std::string, ConfigValue> values_;
    std::string filename_;
    std::mutex mutex_;
};

}
