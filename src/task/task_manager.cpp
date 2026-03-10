#include "task/task_manager.h"
#include "task/task_base.h"
#include <algorithm>
#include <ctime>

namespace mmo {
namespace task {

TaskManager::TaskManager() = default;

TaskManager::~TaskManager() {
    Shutdown();
}

bool TaskManager::Initialize(ITaskRewardHandler* reward_handler, ITaskStorage* storage) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    reward_handler_ = reward_handler;
    storage_ = storage;
    initialized_ = true;
    
    return true;
}

void TaskManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    player_tasks_.clear();
    task_configs_.clear();
    player_levels_.clear();
    initialized_ = false;
}

void TaskManager::LoadTaskConfigs(const std::map<int32_t, TaskConfig>& configs) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [id, config] : configs) {
        task_configs_[id] = config;
    }
}

void TaskManager::AddTaskConfig(const TaskConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    task_configs_[config.task_id] = config;
}

void TaskManager::ClearTaskConfigs() {
    std::lock_guard<std::mutex> lock(mutex_);
    task_configs_.clear();
}

bool TaskManager::AcceptTask(uint64_t player_id, int32_t task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto config_it = task_configs_.find(task_id);
    if (config_it == task_configs_.end()) {
        return false;
    }
    
    auto& player_task_map = player_tasks_[player_id];
    auto task_it = player_task_map.find(task_id);
    
    if (task_it != player_task_map.end()) {
        return false;
    }
    
    auto task = TaskFactory::CreateTask(config_it->second);
    if (!task->Accept(player_id)) {
        return false;
    }
    
    player_task_map[task_id] = task;
    
    if (storage_) {
        storage_->SavePlayerTask(player_id, task->GetPlayerTask());
    }
    
    return true;
}

bool TaskManager::CompleteTask(uint64_t player_id, int32_t task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto task = GetTask(player_id, task_id);
    if (!task) {
        return false;
    }
    
    if (!task->Complete()) {
        return false;
    }
    
    if (storage_) {
        storage_->SavePlayerTask(player_id, task->GetPlayerTask());
    }
    
    return true;
}

bool TaskManager::ClaimReward(uint64_t player_id, int32_t task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto task = GetTask(player_id, task_id);
    if (!task) {
        return false;
    }
    
    if (!task->CanClaim()) {
        return false;
    }
    
    const auto& config = task->GetConfig();
    
    if (reward_handler_ && !config.rewards.empty()) {
        if (!reward_handler_->GiveRewards(player_id, config.rewards)) {
            return false;
        }
    }
    
    if (!task->Claim()) {
        return false;
    }
    
    if (storage_) {
        storage_->SavePlayerTask(player_id, task->GetPlayerTask());
    }
    
    return true;
}

bool TaskManager::AbandonTask(uint64_t player_id, int32_t task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& player_task_map = player_tasks_[player_id];
    auto task_it = player_task_map.find(task_id);
    
    if (task_it == player_task_map.end()) {
        return false;
    }
    
    auto& task = task_it->second;
    if (task->GetStatus() == TaskStatus::COMPLETED || 
        task->GetStatus() == TaskStatus::CLAIMED) {
        return false;
    }
    
    player_task_map.erase(task_it);
    
    if (storage_) {
        storage_->DeletePlayerTask(player_id, task_id);
    }
    
    return true;
}

void TaskManager::OnEvent(const TaskEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_tasks_.find(event.player_id);
    if (it == player_tasks_.end()) {
        return;
    }
    
    for (auto& [task_id, task] : it->second) {
        ProcessEventForTask(event, task);
    }
    
    CheckAutoComplete(event.player_id);
}

void TaskManager::OnEvent(uint64_t player_id, TaskConditionType type, int32_t param1, int32_t param2, int32_t count) {
    TaskEvent event;
    event.player_id = player_id;
    event.condition_type = type;
    event.param1 = param1;
    event.param2 = param2;
    event.count = count;
    
    OnEvent(event);
}

void TaskManager::ProcessEventForTask(const TaskEvent& event, TaskPtr task) {
    if (!task || !task->IsInProgress()) {
        return;
    }
    
    task->UpdateProgress(event.condition_type, event.param1, event.param2, event.count);
    
    if (storage_ && task->GetStatus() == TaskStatus::COMPLETED) {
        storage_->SavePlayerTask(event.player_id, task->GetPlayerTask());
    }
}

TaskManager::TaskPtr TaskManager::GetTask(uint64_t player_id, int32_t task_id) {
    auto player_it = player_tasks_.find(player_id);
    if (player_it == player_tasks_.end()) {
        return nullptr;
    }
    
    auto task_it = player_it->second.find(task_id);
    if (task_it == player_it->second.end()) {
        return nullptr;
    }
    
    return task_it->second;
}

TaskManager::TaskList TaskManager::GetPlayerTasks(uint64_t player_id) {
    TaskList result;
    
    auto it = player_tasks_.find(player_id);
    if (it == player_tasks_.end()) {
        return result;
    }
    
    for (const auto& [id, task] : it->second) {
        result.push_back(task);
    }
    
    return result;
}

TaskManager::TaskList TaskManager::GetPlayerTasksByType(uint64_t player_id, TaskType type) {
    TaskList result;
    
    auto tasks = GetPlayerTasks(player_id);
    for (const auto& task : tasks) {
        if (task->GetTaskType() == type) {
            result.push_back(task);
        }
    }
    
    return result;
}

TaskManager::TaskList TaskManager::GetPlayerTasksByStatus(uint64_t player_id, TaskStatus status) {
    TaskList result;
    
    auto tasks = GetPlayerTasks(player_id);
    for (const auto& task : tasks) {
        if (task->GetStatus() == status) {
            result.push_back(task);
        }
    }
    
    return result;
}

TaskManager::TaskList TaskManager::GetAvailableTasks(uint64_t player_id) {
    TaskList result;
    
    auto level_it = player_levels_.find(player_id);
    int32_t player_level = level_it != player_levels_.end() ? level_it->second : 1;
    
    for (const auto& [id, config] : task_configs_) {
        auto existing_task = GetTask(player_id, id);
        if (existing_task) {
            continue;
        }
        
        if (config.min_level > player_level) {
            continue;
        }
        
        if (config.prev_task_id > 0) {
            auto prev_task = GetTask(player_id, config.prev_task_id);
            if (!prev_task || !prev_task->IsCompleted()) {
                continue;
            }
        }
        
        auto task = TaskFactory::CreateTask(config);
        if (task->CanAccept(player_id)) {
            result.push_back(task);
        }
    }
    
    return result;
}

TaskManager::TaskList TaskManager::GetInProgressTasks(uint64_t player_id) {
    return GetPlayerTasksByStatus(player_id, TaskStatus::IN_PROGRESS);
}

TaskManager::TaskList TaskManager::GetCompletedTasks(uint64_t player_id) {
    TaskList result;
    
    auto tasks = GetPlayerTasks(player_id);
    for (const auto& task : tasks) {
        if (task->GetStatus() == TaskStatus::COMPLETED || 
            task->GetStatus() == TaskStatus::CLAIMED) {
            result.push_back(task);
        }
    }
    
    return result;
}

TaskManager::TaskList TaskManager::GetClaimableTasks(uint64_t player_id) {
    return GetPlayerTasksByStatus(player_id, TaskStatus::COMPLETED);
}

bool TaskManager::HasTask(uint64_t player_id, int32_t task_id) {
    return GetTask(player_id, task_id) != nullptr;
}

bool TaskManager::IsTaskCompleted(uint64_t player_id, int32_t task_id) {
    auto task = GetTask(player_id, task_id);
    return task && task->IsCompleted();
}

bool TaskManager::IsTaskClaimed(uint64_t player_id, int32_t task_id) {
    auto task = GetTask(player_id, task_id);
    return task && task->GetStatus() == TaskStatus::CLAIMED;
}

void TaskManager::OnDailyReset() {
    ResetTasksByType(TaskType::DAILY);
}

void TaskManager::OnWeeklyReset() {
    ResetTasksByType(TaskType::WEEKLY);
}

void TaskManager::OnMonthlyReset() {
    ResetTasksByType(TaskType::MONTHLY);
}

void TaskManager::OnSeasonReset() {
    ResetTasksByType(TaskType::SEASON);
}

void TaskManager::ResetTasksByType(TaskType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [player_id, task_map] : player_tasks_) {
        for (auto& [task_id, task] : task_map) {
            if (task->GetTaskType() == type && task->IsResettable()) {
                task->Reset();
                
                if (storage_) {
                    storage_->SavePlayerTask(player_id, task->GetPlayerTask());
                }
            }
        }
    }
}

void TaskManager::UpdatePlayerLevel(uint64_t player_id, int32_t level) {
    std::lock_guard<std::mutex> lock(mutex_);
    player_levels_[player_id] = level;
    
    CheckAutoAccept(player_id);
}

void TaskManager::CheckAutoAccept(uint64_t player_id) {
    auto available_tasks = GetAvailableTasks(player_id);
    
    for (const auto& task : available_tasks) {
        if (task->GetConfig().auto_accept) {
            AcceptTask(player_id, task->GetTaskId());
        }
    }
}

void TaskManager::CheckAutoComplete(uint64_t player_id) {
    auto in_progress = GetInProgressTasks(player_id);
    
    for (const auto& task : in_progress) {
        if (task->CanComplete() && task->GetConfig().auto_complete) {
            CompleteTask(player_id, task->GetTaskId());
        }
    }
}

void TaskManager::LoadPlayerData(uint64_t player_id) {
    if (!storage_) {
        return;
    }
    
    std::vector<PlayerTask> tasks;
    if (storage_->LoadPlayerTasks(player_id, tasks)) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& player_task_map = player_tasks_[player_id];
        player_task_map.clear();
        
        for (const auto& player_task : tasks) {
            auto config_it = task_configs_.find(player_task.task_id);
            if (config_it != task_configs_.end()) {
                auto task = TaskFactory::CreateTask(config_it->second, player_task);
                player_task_map[player_task.task_id] = task;
            }
        }
    }
}

void TaskManager::SavePlayerData(uint64_t player_id) {
    if (!storage_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_tasks_.find(player_id);
    if (it == player_tasks_.end()) {
        return;
    }
    
    for (const auto& [task_id, task] : it->second) {
        storage_->SavePlayerTask(player_id, task->GetPlayerTask());
    }
}

void TaskManager::ClearPlayerData(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    player_tasks_.erase(player_id);
    player_levels_.erase(player_id);
    
    if (storage_) {
        storage_->ClearPlayerTasks(player_id);
    }
}

int32_t TaskManager::GetTaskCount(uint64_t player_id, TaskType type, TaskStatus status) {
    auto tasks = GetPlayerTasksByType(player_id, type);
    
    int32_t count = 0;
    for (const auto& task : tasks) {
        if (task->GetStatus() == status) {
            ++count;
        }
    }
    
    return count;
}

int32_t TaskManager::GetCompletedCount(uint64_t player_id, TaskType type) {
    auto tasks = GetPlayerTasksByType(player_id, type);
    
    int32_t count = 0;
    for (const auto& task : tasks) {
        if (task->IsCompleted()) {
            ++count;
        }
    }
    
    return count;
}

const TaskConfig* TaskManager::GetTaskConfig(int32_t task_id) const {
    auto it = task_configs_.find(task_id);
    if (it != task_configs_.end()) {
        return &it->second;
    }
    return nullptr;
}

int64_t TaskManager::GetNextDailyResetTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    struct tm tomorrow = *tm_now;
    tomorrow.tm_hour = 0;
    tomorrow.tm_min = 0;
    tomorrow.tm_sec = 0;
    tomorrow.tm_mday += 1;
    
    return mktime(&tomorrow);
}

int64_t TaskManager::GetNextWeeklyResetTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    int days_until_monday = (7 - tm_now->tm_wday + 1) % 7;
    if (days_until_monday == 0) days_until_monday = 7;
    
    struct tm next_monday = *tm_now;
    next_monday.tm_hour = 0;
    next_monday.tm_min = 0;
    next_monday.tm_sec = 0;
    next_monday.tm_mday += days_until_monday;
    
    return mktime(&next_monday);
}

int64_t TaskManager::GetNextMonthlyResetTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    struct tm next_month = *tm_now;
    next_month.tm_hour = 0;
    next_month.tm_min = 0;
    next_month.tm_sec = 0;
    next_month.tm_mday = 1;
    next_month.tm_mon += 1;
    
    return mktime(&next_month);
}

}
}
