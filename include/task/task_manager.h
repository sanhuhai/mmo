#pragma once

#include "task_base.h"
#include "task_data.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

namespace mmo {
namespace task {

struct TaskEvent {
    uint64_t player_id = 0;
    TaskConditionType condition_type = TaskConditionType::NONE;
    int32_t param1 = 0;
    int32_t param2 = 0;
    int32_t count = 1;
    std::string custom_data;
};

class ITaskRewardHandler {
public:
    virtual ~ITaskRewardHandler() = default;
    
    virtual bool GiveReward(uint64_t player_id, const TaskReward& reward) = 0;
    virtual bool GiveRewards(uint64_t player_id, const std::vector<TaskReward>& rewards) = 0;
    virtual bool CanGiveReward(uint64_t player_id, const TaskReward& reward) = 0;
};

class ITaskStorage {
public:
    virtual ~ITaskStorage() = default;
    
    virtual bool SavePlayerTask(uint64_t player_id, const PlayerTask& task) = 0;
    virtual bool LoadPlayerTasks(uint64_t player_id, std::vector<PlayerTask>& tasks) = 0;
    virtual bool DeletePlayerTask(uint64_t player_id, int32_t task_id) = 0;
    virtual bool ClearPlayerTasks(uint64_t player_id) = 0;
};

class TaskManager {
public:
    using TaskPtr = ITask::Ptr;
    using TaskMap = std::unordered_map<int32_t, TaskPtr>;
    using TaskList = std::vector<TaskPtr>;
    
    TaskManager();
    ~TaskManager();
    
    bool Initialize(ITaskRewardHandler* reward_handler, ITaskStorage* storage = nullptr);
    void Shutdown();
    
    void LoadTaskConfigs(const std::map<int32_t, TaskConfig>& configs);
    void AddTaskConfig(const TaskConfig& config);
    void ClearTaskConfigs();
    
    bool AcceptTask(uint64_t player_id, int32_t task_id);
    bool CompleteTask(uint64_t player_id, int32_t task_id);
    bool ClaimReward(uint64_t player_id, int32_t task_id);
    bool AbandonTask(uint64_t player_id, int32_t task_id);
    
    void OnEvent(const TaskEvent& event);
    void OnEvent(uint64_t player_id, TaskConditionType type, int32_t param1 = 0, int32_t param2 = 0, int32_t count = 1);
    
    TaskPtr GetTask(uint64_t player_id, int32_t task_id);
    TaskList GetPlayerTasks(uint64_t player_id);
    TaskList GetPlayerTasksByType(uint64_t player_id, TaskType type);
    TaskList GetPlayerTasksByStatus(uint64_t player_id, TaskStatus status);
    
    TaskList GetAvailableTasks(uint64_t player_id);
    TaskList GetInProgressTasks(uint64_t player_id);
    TaskList GetCompletedTasks(uint64_t player_id);
    TaskList GetClaimableTasks(uint64_t player_id);
    
    bool HasTask(uint64_t player_id, int32_t task_id);
    bool IsTaskCompleted(uint64_t player_id, int32_t task_id);
    bool IsTaskClaimed(uint64_t player_id, int32_t task_id);
    
    void OnDailyReset();
    void OnWeeklyReset();
    void OnMonthlyReset();
    void OnSeasonReset();
    
    void UpdatePlayerLevel(uint64_t player_id, int32_t level);
    void CheckAutoAccept(uint64_t player_id);
    void CheckAutoComplete(uint64_t player_id);
    
    void LoadPlayerData(uint64_t player_id);
    void SavePlayerData(uint64_t player_id);
    void ClearPlayerData(uint64_t player_id);
    
    int32_t GetTaskCount(uint64_t player_id, TaskType type, TaskStatus status);
    int32_t GetCompletedCount(uint64_t player_id, TaskType type);
    
    const TaskConfig* GetTaskConfig(int32_t task_id) const;
    
    void SetRewardHandler(ITaskRewardHandler* handler) { reward_handler_ = handler; }
    void SetStorage(ITaskStorage* storage) { storage_ = storage; }
    
private:
    void ProcessEventForTask(const TaskEvent& event, TaskPtr task);
    void ResetTasksByType(TaskType type);
    
    int64_t GetNextDailyResetTime() const;
    int64_t GetNextWeeklyResetTime() const;
    int64_t GetNextMonthlyResetTime() const;
    
private:
    std::map<int32_t, TaskConfig> task_configs_;
    std::unordered_map<uint64_t, TaskMap> player_tasks_;
    
    ITaskRewardHandler* reward_handler_ = nullptr;
    ITaskStorage* storage_ = nullptr;
    
    std::mutex mutex_;
    bool initialized_ = false;
    
    std::unordered_map<uint64_t, int32_t> player_levels_;
};

}
}
