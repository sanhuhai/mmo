#pragma once

#include "task_data.h"
#include "task_types.h"
#include "task_manager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

struct sqlite3;
struct sqlite3_stmt;

namespace mmo {
namespace task {

class TaskDBLoader {
public:
    using ProgressCallback = std::function<void(const std::string& table, int current, int total)>;
    
    TaskDBLoader();
    ~TaskDBLoader();
    
    bool Open(const std::string& db_path);
    void Close();
    
    bool LoadAllTasks(std::map<int32_t, TaskConfig>& configs);
    bool LoadTasksByType(TaskType type, std::map<int32_t, TaskConfig>& configs);
    bool LoadTasksFromTable(const std::string& table_name, std::map<int32_t, TaskConfig>& configs);
    
    bool LoadPlayerTasks(uint64_t player_id, std::vector<PlayerTask>& tasks);
    bool SavePlayerTask(uint64_t player_id, const PlayerTask& task);
    bool DeletePlayerTask(uint64_t player_id, int32_t task_id);
    bool ClearPlayerTasks(uint64_t player_id);
    
    void SetProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }
    
    const std::string& GetLastError() const { return last_error_; }
    
    std::vector<std::string> GetTableNames();
    bool TableExists(const std::string& table_name);
    
    static TaskConfig ParseTaskConfigFromBinary(const std::vector<uint8_t>& data, size_t& offset);
    static PlayerTask ParsePlayerTaskFromBinary(const std::vector<uint8_t>& data, size_t& offset);
    
private:
    bool ParseTaskRow(void* stmt, TaskConfig& config);
    bool ParsePlayerTaskRow(void* stmt, PlayerTask& task);
    
    std::vector<uint8_t> SerializePlayerTask(const PlayerTask& task);
    
    TaskCondition ParseCondition(const std::string& str);
    TaskReward ParseReward(const std::string& str);
    std::map<std::string, std::string> ParseExtraParams(const std::string& str);
    
private:
    sqlite3* db_;
    std::string last_error_;
    ProgressCallback progress_callback_;
    std::string db_path_;
};

class TaskDBStorage : public ITaskStorage {
public:
    TaskDBStorage(const std::string& db_path);
    ~TaskDBStorage() override;
    
    bool Initialize();
    
    bool SavePlayerTask(uint64_t player_id, const PlayerTask& task) override;
    bool LoadPlayerTasks(uint64_t player_id, std::vector<PlayerTask>& tasks) override;
    bool DeletePlayerTask(uint64_t player_id, int32_t task_id) override;
    bool ClearPlayerTasks(uint64_t player_id) override;
    
    const std::string& GetLastError() const { return last_error_; }
    
private:
    bool CreatePlayerTaskTable();
    
private:
    std::string db_path_;
    sqlite3* db_;
    std::string last_error_;
};

}
}
