#include "item/item_manager.h"
#include <chrono>
#include <iostream>

namespace mmo {
namespace item {

ItemManager::ItemManager() : initialized_(false), next_instance_id_(1) {
}

ItemManager::~ItemManager() {
}

ItemManager& ItemManager::Instance() {
    static ItemManager instance;
    return instance;
}

bool ItemManager::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return true;
    }
    
    // Initialize item templates (examples)
    Item health_potion;
    health_potion.item_id = 1001;
    health_potion.name = "Health Potion";
    health_potion.description = "Restores 100 health points";
    health_potion.type = ITEM_TYPE_CONSUMABLE;
    health_potion.max_stack = 99;
    health_potion.price = 50;
    AddItemTemplate(health_potion);
    
    Item mana_potion;
    mana_potion.item_id = 1002;
    mana_potion.name = "Mana Potion";
    mana_potion.description = "Restores 100 mana points";
    mana_potion.type = ITEM_TYPE_CONSUMABLE;
    mana_potion.max_stack = 99;
    mana_potion.price = 50;
    AddItemTemplate(mana_potion);
    
    // Register item use callbacks
    RegisterItemUseCallback(1001, [](const PlayerItem& item, std::uint64_t owner_id) {
        // Implement health potion usage logic
        // This is just an example, should call player system to increase health
        std::cout << "Player " << owner_id << " used Health Potion" << std::endl;
        return USE_RESULT_SUCCESS;
    });
    
    RegisterItemUseCallback(1002, [](const PlayerItem& item, std::uint64_t owner_id) {
        // Implement mana potion usage logic
        std::cout << "Player " << owner_id << " used Mana Potion" << std::endl;
        return USE_RESULT_SUCCESS;
    });
    
    initialized_ = true;
    return true;
}

void ItemManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }
    
    item_templates_.clear();
    player_items_.clear();
    item_use_callbacks_.clear();
    next_instance_id_ = 1;
    initialized_ = false;
}

bool ItemManager::AddItemTemplate(const Item& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    item_templates_[item.item_id] = item;
    return true;
}

const Item* ItemManager::GetItemTemplate(std::uint32_t item_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = item_templates_.find(item_id);
    if (it != item_templates_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ItemManager::AddItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if item template exists
    auto item_template_it = item_templates_.find(item_id);
    if (item_template_it == item_templates_.end()) {
        return false;
    }
    
    const Item& item_template = item_template_it->second;
    
    // Check if player's item list exists
    auto& player_items = player_items_[owner_id];
    
    // Try to stack to existing item
    if (item_template.max_stack > 1) {
        for (auto& player_item : player_items) {
            if (player_item.item_id == item_id) {
                player_item.quantity += quantity;
                return true;
            }
        }
    }
    
    // Create new items
    for (int i = 0; i < quantity; ++i) {
        PlayerItem new_item;
        new_item.item_instance_id = GenerateItemInstanceId();
        new_item.item_id = item_id;
        new_item.quantity = 1;
        new_item.owner_id = owner_id;
        new_item.created_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        player_items.push_back(new_item);
    }
    
    return true;
}

bool ItemManager::RemoveItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it == player_items_.end()) {
        return false;
    }
    
    auto& items = player_it->second;
    int removed = 0;
    
    for (auto it = items.begin(); it != items.end() && removed < quantity; ) {
        if (it->item_id == item_id) {
            if (it->quantity > 1) {
                it->quantity -= 1;
                removed += 1;
                ++it;
            } else {
                it = items.erase(it);
                removed += 1;
            }
        } else {
            ++it;
        }
    }
    
    return removed >= quantity;
}

bool ItemManager::RemoveItemByInstanceId(std::uint64_t owner_id, std::uint64_t item_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it == player_items_.end()) {
        return false;
    }
    
    auto& items = player_it->second;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->item_instance_id == item_instance_id) {
            items.erase(it);
            return true;
        }
    }
    
    return false;
}

int ItemManager::UseItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it == player_items_.end()) {
        return USE_RESULT_ITEM_NOT_FOUND;
    }
    
    auto& items = player_it->second;
    int used = 0;
    
    for (auto it = items.begin(); it != items.end() && used < quantity; ) {
        if (it->item_id == item_id) {
            // Check if there's a use callback
            auto callback_it = item_use_callbacks_.find(item_id);
            if (callback_it != item_use_callbacks_.end()) {
                int result = callback_it->second(*it, owner_id);
                if (result != USE_RESULT_SUCCESS) {
                    return result;
                }
            }
            
            // Remove used item
            if (it->quantity > 1) {
                it->quantity -= 1;
                used += 1;
                ++it;
            } else {
                it = items.erase(it);
                used += 1;
            }
        } else {
            ++it;
        }
    }
    
    if (used == 0) {
        return USE_RESULT_ITEM_NOT_FOUND;
    }
    
    return USE_RESULT_SUCCESS;
}

int ItemManager::UseItemByInstanceId(std::uint64_t owner_id, std::uint64_t item_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it == player_items_.end()) {
        return USE_RESULT_ITEM_NOT_FOUND;
    }
    
    auto& items = player_it->second;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->item_instance_id == item_instance_id) {
            // Check if there's a use callback
            auto callback_it = item_use_callbacks_.find(it->item_id);
            if (callback_it != item_use_callbacks_.end()) {
                int result = callback_it->second(*it, owner_id);
                if (result != USE_RESULT_SUCCESS) {
                    return result;
                }
            }
            
            // Remove used item
            items.erase(it);
            return USE_RESULT_SUCCESS;
        }
    }
    
    return USE_RESULT_ITEM_NOT_FOUND;
}

std::vector<PlayerItem> ItemManager::GetPlayerItems(std::uint64_t owner_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it != player_items_.end()) {
        return player_it->second;
    }
    
    return std::vector<PlayerItem>();
}

std::int32_t ItemManager::GetItemCount(std::uint64_t owner_id, std::uint32_t item_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto player_it = player_items_.find(owner_id);
    if (player_it == player_items_.end()) {
        return 0;
    }
    
    int count = 0;
    for (const auto& item : player_it->second) {
        if (item.item_id == item_id) {
            count += item.quantity;
        }
    }
    
    return count;
}

void ItemManager::RegisterItemUseCallback(std::uint32_t item_id, ItemUseCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    item_use_callbacks_[item_id] = callback;
}

void ItemManager::UnregisterItemUseCallback(std::uint32_t item_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    item_use_callbacks_.erase(item_id);
}

std::uint64_t ItemManager::GenerateItemInstanceId() {
    std::lock_guard<std::mutex> lock(mutex_);
    return next_instance_id_++;
}

} // namespace item
} // namespace mmo