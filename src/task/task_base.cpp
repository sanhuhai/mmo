#include "task/task_base.h"
#include <algorithm>
#include <ctime>

namespace mmo {
namespace task {

BaseTask::BaseTask(const TaskConfig& config, const PlayerTask& player_task)
    : config_(config)
    , player_task_(player_task) {
    if (player_task_.task_id == 0) {
        player_task_.task_id = config_.task_id;
    }
    
    if (player_task_.progress.empty() && !config_.conditions.empty()) {
        player_task_.progress.resize(config_.conditions.size(), 0);
    }
}

bool BaseTask::CanAccept(uint64_t player_id) const {
    if (player_task_.status != TaskStatus::LOCKED && 
        player_task_.status != TaskStatus::EXPIRED) {
        return false;
    }
    
    if (config_.prev_task_id > 0) {
        return false;
    }
    
    return true;
}

bool BaseTask::Accept(uint64_t player_id) {
    if (!CanAccept(player_id)) {
        return false;
    }
    
    player_task_.player_id = player_id;
    player_task_.status = TaskStatus::IN_PROGRESS;
    player_task_.accept_time = time(nullptr);
    player_task_.progress.clear();
    player_task_.progress.resize(config_.conditions.size(), 0);
    player_task_.current_count = 0;
    
    CalculateExpireTime();
    
    OnAccept();
    
    return true;
}

bool BaseTask::CanComplete() const {
    if (player_task_.status != TaskStatus::IN_PROGRESS) {
        return false;
    }
    
    if (IsExpired()) {
        return false;
    }
    
    return AreAllConditionsComplete();
}

bool BaseTask::Complete() {
    if (!CanComplete()) {
        return false;
    }
    
    player_task_.status = TaskStatus::COMPLETED;
    player_task_.complete_time = time(nullptr);
    
    OnComplete();
    
    return true;
}

bool BaseTask::CanClaim() const {
    return player_task_.status == TaskStatus::COMPLETED;
}

bool BaseTask::Claim() {
    if (!CanClaim()) {
        return false;
    }
    
    player_task_.status = TaskStatus::CLAIMED;
    player_task_.claim_time = time(nullptr);
    
    if (config_.repeatable) {
        player_task_.repeat_count++;
    }
    
    OnClaim();
    
    return true;
}

bool BaseTask::IsExpired() const {
    if (player_task_.expire_time == 0) {
        return false;
    }
    return time(nullptr) > player_task_.expire_time;
}

bool BaseTask::IsResettable() const {
    return TaskTypeToResetType(config_.type) != TaskResetType::NONE;
}

void BaseTask::Reset() {
    player_task_.status = TaskStatus::LOCKED;
    player_task_.progress.clear();
    player_task_.progress.resize(config_.conditions.size(), 0);
    player_task_.current_count = 0;
    player_task_.accept_time = 0;
    player_task_.complete_time = 0;
    player_task_.claim_time = 0;
    player_task_.expire_time = 0;
    
    OnReset();
}

bool BaseTask::IsInProgress() const {
    return player_task_.status == TaskStatus::IN_PROGRESS;
}

bool BaseTask::IsCompleted() const {
    return player_task_.status == TaskStatus::COMPLETED || 
           player_task_.status == TaskStatus::CLAIMED;
}

void BaseTask::UpdateProgress(TaskConditionType type, int32_t param1, int32_t param2, int32_t count) {
    if (player_task_.status != TaskStatus::IN_PROGRESS) {
        return;
    }
    
    int32_t index = FindConditionIndex(type, param1, param2);
    if (index < 0) {
        return;
    }
    
    int32_t target = config_.conditions[index].target_count;
    int32_t current = player_task_.progress[index];
    
    current = std::min(current + count, target);
    player_task_.progress[index] = current;
    
    if (AreAllConditionsComplete() && config_.auto_complete) {
        Complete();
    }
}

int32_t BaseTask::GetProgress(int32_t condition_index) const {
    if (condition_index < 0 || condition_index >= static_cast<int32_t>(player_task_.progress.size())) {
        return 0;
    }
    return player_task_.progress[condition_index];
}

float BaseTask::GetProgressPercent() const {
    if (config_.conditions.empty()) {
        return 0.0f;
    }
    
    float total = 0.0f;
    for (size_t i = 0; i < config_.conditions.size(); ++i) {
        int32_t current = player_task_.progress[i];
        int32_t target = config_.conditions[i].target_count;
        if (target > 0) {
            total += static_cast<float>(current) / static_cast<float>(target);
        }
    }
    
    return total / static_cast<float>(config_.conditions.size());
}

std::vector<TaskProgress> BaseTask::GetAllProgress() const {
    std::vector<TaskProgress> progress_list;
    progress_list.reserve(config_.conditions.size());
    
    for (size_t i = 0; i < config_.conditions.size(); ++i) {
        TaskProgress progress;
        progress.condition_type = config_.conditions[i].type;
        progress.param1 = config_.conditions[i].param1;
        progress.param2 = config_.conditions[i].param2;
        progress.current = player_task_.progress[i];
        progress.target = config_.conditions[i].target_count;
        progress_list.push_back(progress);
    }
    
    return progress_list;
}

bool BaseTask::IsConditionComplete(int32_t condition_index) const {
    if (condition_index < 0 || condition_index >= static_cast<int32_t>(config_.conditions.size())) {
        return false;
    }
    
    int32_t current = player_task_.progress[condition_index];
    int32_t target = config_.conditions[condition_index].target_count;
    
    return current >= target;
}

bool BaseTask::AreAllConditionsComplete() const {
    for (size_t i = 0; i < config_.conditions.size(); ++i) {
        if (!IsConditionComplete(static_cast<int32_t>(i))) {
            return false;
        }
    }
    return true;
}

int64_t BaseTask::GetRemainingTime() const {
    if (player_task_.expire_time == 0) {
        return -1;
    }
    
    int64_t now = time(nullptr);
    if (now >= player_task_.expire_time) {
        return 0;
    }
    
    return player_task_.expire_time - now;
}

void BaseTask::CalculateExpireTime() {
    if (config_.time_limit > 0) {
        player_task_.expire_time = time(nullptr) + config_.time_limit;
    } else {
        player_task_.expire_time = 0;
    }
}

int32_t BaseTask::FindConditionIndex(TaskConditionType type, int32_t param1, int32_t param2) const {
    for (size_t i = 0; i < config_.conditions.size(); ++i) {
        const auto& cond = config_.conditions[i];
        if (cond.type == type && 
            (cond.param1 == 0 || cond.param1 == param1) &&
            (cond.param2 == 0 || cond.param2 == param2)) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

int64_t DailyTask::GetRemainingTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    struct tm tomorrow = *tm_now;
    tomorrow.tm_hour = 0;
    tomorrow.tm_min = 0;
    tomorrow.tm_sec = 0;
    tomorrow.tm_mday += 1;
    
    time_t reset_time = mktime(&tomorrow);
    return reset_time - now;
}

int64_t WeeklyTask::GetRemainingTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    int days_until_monday = (7 - tm_now->tm_wday + 1) % 7;
    if (days_until_monday == 0) days_until_monday = 7;
    
    struct tm next_monday = *tm_now;
    next_monday.tm_hour = 0;
    next_monday.tm_min = 0;
    next_monday.tm_sec = 0;
    next_monday.tm_mday += days_until_monday;
    
    time_t reset_time = mktime(&next_monday);
    return reset_time - now;
}

int64_t MonthlyTask::GetRemainingTime() const {
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    struct tm next_month = *tm_now;
    next_month.tm_hour = 0;
    next_month.tm_min = 0;
    next_month.tm_sec = 0;
    next_month.tm_mday = 1;
    next_month.tm_mon += 1;
    
    time_t reset_time = mktime(&next_month);
    return reset_time - now;
}

int64_t SeasonTask::GetRemainingTime() const {
    if (config_.end_time > 0) {
        int64_t remaining = config_.end_time - time(nullptr);
        return remaining > 0 ? remaining : 0;
    }
    return -1;
}

bool MainQuestTask::CanAccept(uint64_t player_id) const {
    if (!BaseTask::CanAccept(player_id)) {
        return false;
    }
    
    if (config_.prev_task_id > 0) {
        return false;
    }
    
    return true;
}

bool AchievementTask::CanClaim() const {
    if (!BaseTask::CanClaim()) {
        return false;
    }
    return true;
}

bool EventTask::CanAccept(uint64_t player_id) const {
    if (!BaseTask::CanAccept(player_id)) {
        return false;
    }
    
    int64_t now = time(nullptr);
    if (config_.start_time > 0 && now < config_.start_time) {
        return false;
    }
    if (config_.end_time > 0 && now > config_.end_time) {
        return false;
    }
    
    return true;
}

bool EventTask::IsExpired() const {
    if (config_.end_time > 0) {
        return time(nullptr) > config_.end_time;
    }
    return BaseTask::IsExpired();
}

int64_t EventTask::GetRemainingTime() const {
    if (config_.end_time > 0) {
        int64_t remaining = config_.end_time - time(nullptr);
        return remaining > 0 ? remaining : 0;
    }
    return BaseTask::GetRemainingTime();
}

ITask::Ptr TaskFactory::CreateTask(const TaskConfig& config, const PlayerTask& player_task) {
    return CreateTask(config.type, config, player_task);
}

ITask::Ptr TaskFactory::CreateTask(TaskType type, const TaskConfig& config, const PlayerTask& player_task) {
    switch (type) {
        case TaskType::DAILY:
            return std::make_shared<DailyTask>(config, player_task);
        case TaskType::WEEKLY:
            return std::make_shared<WeeklyTask>(config, player_task);
        case TaskType::MONTHLY:
            return std::make_shared<MonthlyTask>(config, player_task);
        case TaskType::SEASON:
            return std::make_shared<SeasonTask>(config, player_task);
        case TaskType::MAIN_QUEST:
            return std::make_shared<MainQuestTask>(config, player_task);
        case TaskType::SIDE_QUEST:
            return std::make_shared<SideQuestTask>(config, player_task);
        case TaskType::ACHIEVEMENT:
            return std::make_shared<AchievementTask>(config, player_task);
        case TaskType::EVENT:
            return std::make_shared<EventTask>(config, player_task);
        case TaskType::GUIDE:
            return std::make_shared<GuideTask>(config, player_task);
        default:
            return std::make_shared<BaseTask>(config, player_task);
    }
}

}
}
