#include "task/task_db_loader.h"
#include <sqlite3.h>
#include <cstring>
#include <sstream>

namespace mmo {
namespace task {

TaskDBLoader::TaskDBLoader()
    : db_(nullptr) {
}

TaskDBLoader::~TaskDBLoader() {
    Close();
}

bool TaskDBLoader::Open(const std::string& db_path) {
    Close();
    
    int result = sqlite3_open(db_path.c_str(), &db_);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to open database: " + db_path;
        return false;
    }
    
    db_path_ = db_path;
    return true;
}

void TaskDBLoader::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    db_path_.clear();
}

std::vector<std::string> TaskDBLoader::GetTableNames() {
    std::vector<std::string> tables;
    if (!db_) return tables;
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (name) {
                tables.push_back(name);
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return tables;
}

bool TaskDBLoader::TableExists(const std::string& table_name) {
    if (!db_) return false;
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

bool TaskDBLoader::LoadAllTasks(std::map<int32_t, TaskConfig>& configs) {
    auto tables = GetTableNames();
    
    int total = static_cast<int>(tables.size());
    int current = 0;
    
    for (const auto& table : tables) {
        ++current;
        
        if (progress_callback_) {
            progress_callback_(table, current, total);
        }
        
        if (table.find("task") != std::string::npos || 
            table.find("quest") != std::string::npos ||
            table.find("daily") != std::string::npos ||
            table.find("weekly") != std::string::npos ||
            table.find("main") != std::string::npos ||
            table.find("side") != std::string::npos) {
            LoadTasksFromTable(table, configs);
        }
    }
    
    return !configs.empty();
}

bool TaskDBLoader::LoadTasksByType(TaskType type, std::map<int32_t, TaskConfig>& configs) {
    std::string table_name = TaskTypeToString(type) + "_task";
    
    if (!TableExists(table_name)) {
        table_name = TaskTypeToString(type);
    }
    
    return LoadTasksFromTable(table_name, configs);
}

bool TaskDBLoader::LoadTasksFromTable(const std::string& table_name, std::map<int32_t, TaskConfig>& configs) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    std::string sql = "SELECT * FROM \"" + table_name + "\";";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    int col_count = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TaskConfig config;
        
        for (int i = 0; i < col_count; ++i) {
            const char* col_name = sqlite3_column_name(stmt, i);
            std::string name = col_name ? col_name : "";
            
            if (name == "task_id" || name == "id") {
                config.task_id = sqlite3_column_int(stmt, i);
            } else if (name == "type" || name == "task_type") {
                const char* type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.type = StringToTaskType(type_str ? type_str : "");
            } else if (name == "name") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.name = str ? str : "";
            } else if (name == "description" || name == "desc") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.description = str ? str : "";
            } else if (name == "prev_task_id" || name == "prev_id") {
                config.prev_task_id = sqlite3_column_int(stmt, i);
            } else if (name == "next_task_id" || name == "next_id") {
                config.next_task_id = sqlite3_column_int(stmt, i);
            } else if (name == "min_level") {
                config.min_level = sqlite3_column_int(stmt, i);
            } else if (name == "max_level") {
                config.max_level = sqlite3_column_int(stmt, i);
            } else if (name == "required_quest" || name == "req_quest") {
                config.required_quest = sqlite3_column_int(stmt, i);
            } else if (name == "conditions" || name == "condition") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (str) {
                    std::istringstream iss(str);
                    std::string cond_str;
                    while (std::getline(iss, cond_str, ';')) {
                        if (!cond_str.empty()) {
                            config.conditions.push_back(ParseCondition(cond_str));
                        }
                    }
                }
            } else if (name == "rewards" || name == "reward") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (str) {
                    std::istringstream iss(str);
                    std::string reward_str;
                    while (std::getline(iss, reward_str, ';')) {
                        if (!reward_str.empty()) {
                            config.rewards.push_back(ParseReward(reward_str));
                        }
                    }
                }
            } else if (name == "time_limit") {
                config.time_limit = sqlite3_column_int(stmt, i);
            } else if (name == "start_time") {
                config.start_time = sqlite3_column_int(stmt, i);
            } else if (name == "end_time") {
                config.end_time = sqlite3_column_int(stmt, i);
            } else if (name == "priority") {
                config.priority = sqlite3_column_int(stmt, i);
            } else if (name == "sort_order" || name == "sort") {
                config.sort_order = sqlite3_column_int(stmt, i);
            } else if (name == "auto_accept") {
                config.auto_accept = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "auto_complete") {
                config.auto_complete = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "repeatable") {
                config.repeatable = sqlite3_column_int(stmt, i) != 0;
            } else if (name == "max_repeat_count") {
                config.max_repeat_count = sqlite3_column_int(stmt, i);
            } else if (name == "icon") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.icon = str ? str : "";
            } else if (name == "dialog_start") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.dialog_start = str ? str : "";
            } else if (name == "dialog_complete") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.dialog_complete = str ? str : "";
            } else if (name == "dialog_npc") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                config.dialog_npc = str ? str : "";
            } else if (name == "extra_params" || name == "extra") {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (str) {
                    config.extra_params = ParseExtraParams(str);
                }
            } else if (name == "_binary") {
                const void* blob = sqlite3_column_blob(stmt, i);
                int blob_size = sqlite3_column_bytes(stmt, i);
                if (blob && blob_size > 0) {
                    size_t offset = 0;
                    std::vector<uint8_t> data(static_cast<const uint8_t*>(blob), 
                                             static_cast<const uint8_t*>(blob) + blob_size);
                }
            }
        }
        
        if (config.task_id > 0) {
            configs[config.task_id] = config;
        }
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool TaskDBLoader::LoadPlayerTasks(uint64_t player_id, std::vector<PlayerTask>& tasks) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    const char* sql = "SELECT * FROM player_tasks WHERE player_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement";
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PlayerTask task;
        ParsePlayerTaskRow(stmt, task);
        tasks.push_back(task);
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool TaskDBLoader::SavePlayerTask(uint64_t player_id, const PlayerTask& task) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    const char* sql = "INSERT OR REPLACE INTO player_tasks "
        "(player_id, task_id, status, progress, current_count, accept_time, complete_time, "
        "claim_time, expire_time, repeat_count) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement";
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, task.task_id);
    sqlite3_bind_int(stmt, 3, static_cast<int>(task.status));
    
    std::string progress_str;
    for (size_t i = 0; i < task.progress.size(); ++i) {
        if (i > 0) progress_str += ",";
        progress_str += std::to_string(task.progress[i]);
    }
    sqlite3_bind_text(stmt, 4, progress_str.c_str(), -1, SQLITE_TRANSIENT);
    
    sqlite3_bind_int(stmt, 5, task.current_count);
    sqlite3_bind_int64(stmt, 6, task.accept_time);
    sqlite3_bind_int64(stmt, 7, task.complete_time);
    sqlite3_bind_int64(stmt, 8, task.claim_time);
    sqlite3_bind_int64(stmt, 9, task.expire_time);
    sqlite3_bind_int(stmt, 10, task.repeat_count);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

bool TaskDBLoader::DeletePlayerTask(uint64_t player_id, int32_t task_id) {
    if (!db_) return false;
    
    const char* sql = "DELETE FROM player_tasks WHERE player_id = ? AND task_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, task_id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

bool TaskDBLoader::ClearPlayerTasks(uint64_t player_id) {
    if (!db_) return false;
    
    const char* sql = "DELETE FROM player_tasks WHERE player_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

TaskCondition TaskDBLoader::ParseCondition(const std::string& str) {
    TaskCondition cond;
    
    size_t pos1 = str.find(':');
    if (pos1 == std::string::npos) {
        cond.type = TaskConditionType::CUSTOM;
        cond.custom_param = str;
        return cond;
    }
    
    std::string type_str = str.substr(0, pos1);
    cond.type = static_cast<TaskConditionType>(std::stoi(type_str));
    
    std::string rest = str.substr(pos1 + 1);
    size_t pos2 = rest.find(':');
    
    if (pos2 != std::string::npos) {
        cond.param1 = std::stoi(rest.substr(0, pos2));
        std::string rest2 = rest.substr(pos2 + 1);
        
        size_t pos3 = rest2.find(':');
        if (pos3 != std::string::npos) {
            cond.param2 = std::stoi(rest2.substr(0, pos3));
            cond.target_count = std::stoi(rest2.substr(pos3 + 1));
        } else {
            cond.target_count = std::stoi(rest2);
        }
    } else {
        cond.target_count = std::stoi(rest);
    }
    
    return cond;
}

TaskReward TaskDBLoader::ParseReward(const std::string& str) {
    TaskReward reward;
    
    size_t pos = str.find(':');
    if (pos == std::string::npos) {
        reward.custom_reward = str;
        return reward;
    }
    
    std::string type = str.substr(0, pos);
    std::string value = str.substr(pos + 1);
    
    if (type == "gold") {
        reward.gold = std::stoi(value);
    } else if (type == "exp") {
        reward.exp = std::stoi(value);
    } else if (type == "diamond") {
        reward.diamond = std::stoi(value);
    } else if (type == "item") {
        size_t pos2 = value.find(':');
        if (pos2 != std::string::npos) {
            reward.item_id = std::stoi(value.substr(0, pos2));
            reward.count = std::stoi(value.substr(pos2 + 1));
        } else {
            reward.item_id = std::stoi(value);
            reward.count = 1;
        }
    } else if (type == "reward_id") {
        reward.reward_id = std::stoi(value);
    } else {
        reward.custom_reward = str;
    }
    
    return reward;
}

std::map<std::string, std::string> TaskDBLoader::ParseExtraParams(const std::string& str) {
    std::map<std::string, std::string> params;
    
    std::istringstream iss(str);
    std::string pair;
    
    while (std::getline(iss, pair, ';')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;
        }
    }
    
    return params;
}

bool TaskDBLoader::ParseTaskRow(void* stmt, TaskConfig& config) {
    sqlite3_stmt* s = static_cast<sqlite3_stmt*>(stmt);
    return true;
}

bool TaskDBLoader::ParsePlayerTaskRow(void* stmt, PlayerTask& task) {
    sqlite3_stmt* s = static_cast<sqlite3_stmt*>(stmt);
    
    int col_count = sqlite3_column_count(s);
    
    for (int i = 0; i < col_count; ++i) {
        const char* col_name = sqlite3_column_name(s, i);
        std::string name = col_name ? col_name : "";
        
        if (name == "player_id") {
            task.player_id = sqlite3_column_int64(s, i);
        } else if (name == "task_id") {
            task.task_id = sqlite3_column_int(s, i);
        } else if (name == "status") {
            task.status = static_cast<TaskStatus>(sqlite3_column_int(s, i));
        } else if (name == "progress") {
            const char* str = reinterpret_cast<const char*>(sqlite3_column_text(s, i));
            if (str) {
                std::istringstream iss(str);
                std::string val;
                while (std::getline(iss, val, ',')) {
                    task.progress.push_back(std::stoi(val));
                }
            }
        } else if (name == "current_count") {
            task.current_count = sqlite3_column_int(s, i);
        } else if (name == "accept_time") {
            task.accept_time = sqlite3_column_int64(s, i);
        } else if (name == "complete_time") {
            task.complete_time = sqlite3_column_int64(s, i);
        } else if (name == "claim_time") {
            task.claim_time = sqlite3_column_int64(s, i);
        } else if (name == "expire_time") {
            task.expire_time = sqlite3_column_int64(s, i);
        } else if (name == "repeat_count") {
            task.repeat_count = sqlite3_column_int(s, i);
        }
    }
    
    return true;
}

std::vector<uint8_t> TaskDBLoader::SerializePlayerTask(const PlayerTask& task) {
    std::vector<uint8_t> data;
    return data;
}

TaskDBStorage::TaskDBStorage(const std::string& db_path)
    : db_path_(db_path)
    , db_(nullptr) {
}

TaskDBStorage::~TaskDBStorage() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool TaskDBStorage::Initialize() {
    if (sqlite3_open(db_path_.c_str(), &db_) != SQLITE_OK) {
        last_error_ = "Failed to open database";
        return false;
    }
    
    return CreatePlayerTaskTable();
}

bool TaskDBStorage::CreatePlayerTaskTable() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS player_tasks ("
        "player_id INTEGER NOT NULL, "
        "task_id INTEGER NOT NULL, "
        "status INTEGER NOT NULL, "
        "progress TEXT, "
        "current_count INTEGER DEFAULT 0, "
        "accept_time INTEGER DEFAULT 0, "
        "complete_time INTEGER DEFAULT 0, "
        "claim_time INTEGER DEFAULT 0, "
        "expire_time INTEGER DEFAULT 0, "
        "repeat_count INTEGER DEFAULT 0, "
        "PRIMARY KEY (player_id, task_id)"
        ");";
    
    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
    
    if (result != SQLITE_OK) {
        last_error_ = err_msg ? err_msg : "Failed to create table";
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool TaskDBStorage::SavePlayerTask(uint64_t player_id, const PlayerTask& task) {
    const char* sql = "INSERT OR REPLACE INTO player_tasks "
        "(player_id, task_id, status, progress, current_count, accept_time, complete_time, "
        "claim_time, expire_time, repeat_count) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement";
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, task.task_id);
    sqlite3_bind_int(stmt, 3, static_cast<int>(task.status));
    
    std::string progress_str;
    for (size_t i = 0; i < task.progress.size(); ++i) {
        if (i > 0) progress_str += ",";
        progress_str += std::to_string(task.progress[i]);
    }
    sqlite3_bind_text(stmt, 4, progress_str.c_str(), -1, SQLITE_TRANSIENT);
    
    sqlite3_bind_int(stmt, 5, task.current_count);
    sqlite3_bind_int64(stmt, 6, task.accept_time);
    sqlite3_bind_int64(stmt, 7, task.complete_time);
    sqlite3_bind_int64(stmt, 8, task.claim_time);
    sqlite3_bind_int64(stmt, 9, task.expire_time);
    sqlite3_bind_int(stmt, 10, task.repeat_count);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

bool TaskDBStorage::LoadPlayerTasks(uint64_t player_id, std::vector<PlayerTask>& tasks) {
    const char* sql = "SELECT * FROM player_tasks WHERE player_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = "Failed to prepare statement";
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PlayerTask task;
        
        task.player_id = sqlite3_column_int64(stmt, 0);
        task.task_id = sqlite3_column_int(stmt, 1);
        task.status = static_cast<TaskStatus>(sqlite3_column_int(stmt, 2));
        
        const char* progress_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (progress_str) {
            std::istringstream iss(progress_str);
            std::string val;
            while (std::getline(iss, val, ',')) {
                task.progress.push_back(std::stoi(val));
            }
        }
        
        task.current_count = sqlite3_column_int(stmt, 4);
        task.accept_time = sqlite3_column_int64(stmt, 5);
        task.complete_time = sqlite3_column_int64(stmt, 6);
        task.claim_time = sqlite3_column_int64(stmt, 7);
        task.expire_time = sqlite3_column_int64(stmt, 8);
        task.repeat_count = sqlite3_column_int(stmt, 9);
        
        tasks.push_back(task);
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool TaskDBStorage::DeletePlayerTask(uint64_t player_id, int32_t task_id) {
    const char* sql = "DELETE FROM player_tasks WHERE player_id = ? AND task_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    sqlite3_bind_int(stmt, 2, task_id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

bool TaskDBStorage::ClearPlayerTasks(uint64_t player_id) {
    const char* sql = "DELETE FROM player_tasks WHERE player_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, player_id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

}
}
