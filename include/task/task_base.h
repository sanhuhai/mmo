#pragma once

#include "task_data.h"
#include "task_types.h"
#include <memory>
#include <functional>
#include <ctime>

namespace mmo {
namespace task {

class ITask {
public:
    using Ptr = std::shared_ptr<ITask>;
    using WeakPtr = std::weak_ptr<ITask>;
    
    virtual ~ITask() = default;
    
    virtual int32_t GetTaskId() const = 0;
    virtual TaskType GetTaskType() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual const std::string& GetDescription() const = 0;
    
    virtual const TaskConfig& GetConfig() const = 0;
    virtual const PlayerTask& GetPlayerTask() const = 0;
    
    virtual TaskStatus GetStatus() const = 0;
    virtual void SetStatus(TaskStatus status) = 0;
    
    virtual bool CanAccept(uint64_t player_id) const = 0;
    virtual bool Accept(uint64_t player_id) = 0;
    
    virtual bool CanComplete() const = 0;
    virtual bool Complete() = 0;
    
    virtual bool CanClaim() const = 0;
    virtual bool Claim() = 0;
    
    virtual bool IsExpired() const = 0;
    virtual bool IsResettable() const = 0;
    virtual void Reset() = 0;
    
    virtual bool IsInProgress() const = 0;
    virtual bool IsCompleted() const = 0;
    
    virtual void UpdateProgress(TaskConditionType type, int32_t param1, int32_t param2, int32_t count = 1) = 0;
    virtual int32_t GetProgress(int32_t condition_index = 0) const = 0;
    virtual float GetProgressPercent() const = 0;
    
    virtual std::vector<TaskProgress> GetAllProgress() const = 0;
    
    virtual bool IsConditionComplete(int32_t condition_index) const = 0;
    virtual bool AreAllConditionsComplete() const = 0;
    
    virtual int64_t GetRemainingTime() const = 0;
};

class BaseTask : public ITask {
public:
    BaseTask(const TaskConfig& config, const PlayerTask& player_task = PlayerTask{});
    ~BaseTask() override = default;
    
    int32_t GetTaskId() const override { return config_.task_id; }
    TaskType GetTaskType() const override { return config_.type; }
    const std::string& GetName() const override { return config_.name; }
    const std::string& GetDescription() const override { return config_.description; }
    
    const TaskConfig& GetConfig() const override { return config_; }
    const PlayerTask& GetPlayerTask() const override { return player_task_; }
    
    TaskStatus GetStatus() const override { return player_task_.status; }
    void SetStatus(TaskStatus status) override { player_task_.status = status; }
    
    bool CanAccept(uint64_t player_id) const override;
    bool Accept(uint64_t player_id) override;
    
    bool CanComplete() const override;
    bool Complete() override;
    
    bool CanClaim() const override;
    bool Claim() override;
    
    bool IsExpired() const override;
    bool IsResettable() const override;
    void Reset() override;
    
    bool IsInProgress() const override;
    bool IsCompleted() const override;
    
    void UpdateProgress(TaskConditionType type, int32_t param1, int32_t param2, int32_t count = 1) override;
    int32_t GetProgress(int32_t condition_index = 0) const override;
    float GetProgressPercent() const override;
    
    std::vector<TaskProgress> GetAllProgress() const override;
    
    bool IsConditionComplete(int32_t condition_index) const override;
    bool AreAllConditionsComplete() const override;
    
    int64_t GetRemainingTime() const override;
    
protected:
    virtual void OnAccept() {}
    virtual void OnComplete() {}
    virtual void OnClaim() {}
    virtual void OnReset() {}
    
    void CalculateExpireTime();
    int32_t FindConditionIndex(TaskConditionType type, int32_t param1, int32_t param2) const;
    
protected:
    TaskConfig config_;
    PlayerTask player_task_;
};

class DailyTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return true; }
    int64_t GetRemainingTime() const override;
};

class WeeklyTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return true; }
    int64_t GetRemainingTime() const override;
};

class MonthlyTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return true; }
    int64_t GetRemainingTime() const override;
};

class SeasonTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return true; }
    int64_t GetRemainingTime() const override;
};

class MainQuestTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool CanAccept(uint64_t player_id) const override;
    bool IsResettable() const override { return false; }
};

class SideQuestTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return config_.repeatable; }
};

class AchievementTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return false; }
    bool CanClaim() const override;
};

class EventTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool CanAccept(uint64_t player_id) const override;
    bool IsExpired() const override;
    int64_t GetRemainingTime() const override;
};

class GuideTask : public BaseTask {
public:
    using BaseTask::BaseTask;
    
    bool IsResettable() const override { return false; }
};

class TaskFactory {
public:
    static ITask::Ptr CreateTask(const TaskConfig& config, const PlayerTask& player_task = PlayerTask{});
    static ITask::Ptr CreateTask(TaskType type, const TaskConfig& config, const PlayerTask& player_task = PlayerTask{});
};

}
}
