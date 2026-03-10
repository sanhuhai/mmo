#pragma once

#include "task_types.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <ctime>

namespace mmo {
namespace task {

struct TaskCondition {
    TaskConditionType type = TaskConditionType::NONE;
    int32_t param1 = 0;
    int32_t param2 = 0;
    int32_t target_count = 1;
    std::string custom_param;
    
    bool operator==(const TaskCondition& other) const {
        return type == other.type && 
               param1 == other.param1 && 
               param2 == other.param2;
    }
};

struct TaskReward {
    int32_t reward_id = 0;
    int32_t item_id = 0;
    int32_t count = 0;
    int32_t gold = 0;
    int32_t exp = 0;
    int32_t diamond = 0;
    std::string custom_reward;
    
    bool IsEmpty() const {
        return reward_id == 0 && item_id == 0 && 
               gold == 0 && exp == 0 && diamond == 0;
    }
};

struct TaskConfig {
    int32_t task_id = 0;
    TaskType type = TaskType::UNKNOWN;
    std::string name;
    std::string description;
    
    int32_t prev_task_id = 0;
    int32_t next_task_id = 0;
    
    int32_t min_level = 1;
    int32_t max_level = 0;
    int32_t required_quest = 0;
    
    std::vector<TaskCondition> conditions;
    std::vector<TaskReward> rewards;
    
    int32_t time_limit = 0;
    int32_t start_time = 0;
    int32_t end_time = 0;
    
    int32_t priority = 0;
    int32_t sort_order = 0;
    bool auto_accept = false;
    bool auto_complete = false;
    bool repeatable = false;
    int32_t max_repeat_count = 1;
    
    std::string icon;
    std::string dialog_start;
    std::string dialog_complete;
    std::string dialog_npc;
    
    std::map<std::string, std::string> extra_params;
};

struct PlayerTask {
    int32_t task_id = 0;
    uint64_t player_id = 0;
    TaskStatus status = TaskStatus::LOCKED;
    
    std::vector<int32_t> progress;
    int32_t current_count = 0;
    
    int64_t accept_time = 0;
    int64_t complete_time = 0;
    int64_t claim_time = 0;
    int64_t expire_time = 0;
    
    int32_t repeat_count = 0;
    
    bool IsExpired() const {
        if (expire_time == 0) return false;
        return time(nullptr) > expire_time;
    }
    
    bool CanClaim() const {
        return status == TaskStatus::COMPLETED;
    }
    
    bool IsInProgress() const {
        return status == TaskStatus::IN_PROGRESS;
    }
    
    bool IsCompleted() const {
        return status == TaskStatus::COMPLETED || status == TaskStatus::CLAIMED;
    }
};

struct TaskProgress {
    TaskConditionType condition_type = TaskConditionType::NONE;
    int32_t param1 = 0;
    int32_t param2 = 0;
    int32_t current = 0;
    int32_t target = 1;
    
    bool IsComplete() const {
        return current >= target;
    }
    
    float GetProgress() const {
        if (target <= 0) return 1.0f;
        return static_cast<float>(current) / static_cast<float>(target);
    }
};

}
}
