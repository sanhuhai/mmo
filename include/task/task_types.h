#pragma once

#include <string>
#include <cstdint>

namespace mmo {
namespace task {

enum class TaskType : uint8_t {
    DAILY = 0,          
    WEEKLY = 1,         
    MONTHLY = 2,        
    SEASON = 3,         
    MAIN_QUEST = 4,     
    SIDE_QUEST = 5,     
    ACHIEVEMENT = 6,    
    EVENT = 7,          
    GUIDE = 8,          
    UNKNOWN = 255
};

enum class TaskStatus : uint8_t {
    LOCKED = 0,         
    UNLOCKED = 1,       
    IN_PROGRESS = 2,    
    COMPLETED = 3,      
    CLAIMED = 4,        
    EXPIRED = 5,        
    FAILED = 6          
};

enum class TaskConditionType : uint16_t {
    NONE = 0,
    KILL_MONSTER = 1,
    COLLECT_ITEM = 2,
    TALK_TO_NPC = 3,
    REACH_LEVEL = 4,
    REACH_LOCATION = 5,
    COMPLETE_DUNGEON = 6,
    WIN_PVP = 7,
    LOGIN_DAYS = 8,
    CONSUME_ITEM = 9,
    UPGRADE_EQUIPMENT = 10,
    LEARN_SKILL = 11,
    JOIN_GUILD = 12,
    ADD_FRIEND = 13,
    CHAT_MESSAGE = 14,
    TRADE = 15,
    SPEND_GOLD = 16,
    SPEND_DIAMOND = 17,
    RECHARGE = 18,
    SHARE = 19,
    INVITE_FRIEND = 20,
    CUSTOM = 1000
};

enum class TaskResetType : uint8_t {
    NONE = 0,           
    DAILY_RESET = 1,    
    WEEKLY_RESET = 2,   
    MONTHLY_RESET = 3,  
    SEASON_RESET = 4    
};

inline TaskType StringToTaskType(const std::string& str) {
    if (str == "daily") return TaskType::DAILY;
    if (str == "weekly") return TaskType::WEEKLY;
    if (str == "monthly") return TaskType::MONTHLY;
    if (str == "season") return TaskType::SEASON;
    if (str == "main" || str == "main_quest") return TaskType::MAIN_QUEST;
    if (str == "side" || str == "side_quest") return TaskType::SIDE_QUEST;
    if (str == "achievement") return TaskType::ACHIEVEMENT;
    if (str == "event") return TaskType::EVENT;
    if (str == "guide") return TaskType::GUIDE;
    return TaskType::UNKNOWN;
}

inline std::string TaskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::DAILY: return "daily";
        case TaskType::WEEKLY: return "weekly";
        case TaskType::MONTHLY: return "monthly";
        case TaskType::SEASON: return "season";
        case TaskType::MAIN_QUEST: return "main";
        case TaskType::SIDE_QUEST: return "side";
        case TaskType::ACHIEVEMENT: return "achievement";
        case TaskType::EVENT: return "event";
        case TaskType::GUIDE: return "guide";
        default: return "unknown";
    }
}

inline TaskResetType TaskTypeToResetType(TaskType type) {
    switch (type) {
        case TaskType::DAILY: return TaskResetType::DAILY_RESET;
        case TaskType::WEEKLY: return TaskResetType::WEEKLY_RESET;
        case TaskType::MONTHLY: return TaskResetType::MONTHLY_RESET;
        case TaskType::SEASON: return TaskResetType::SEASON_RESET;
        default: return TaskResetType::NONE;
    }
}

}
}
