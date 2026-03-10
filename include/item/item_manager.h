#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>

namespace mmo {
namespace item {

// Item type enum
enum ItemType {
    ITEM_TYPE_CONSUMABLE = 0,    // Consumable
    ITEM_TYPE_EQUIPMENT = 1,     // Equipment
    ITEM_TYPE_MATERIAL = 2,      // Material
    ITEM_TYPE_CURRENCY = 3,      // Currency
    ITEM_TYPE_QUEST = 4,         // Quest item
    ITEM_TYPE_SPECIAL = 5        // Special item
};

// Item use result enum
enum UseResult {
    USE_RESULT_SUCCESS = 0,            // Success
    USE_RESULT_ITEM_NOT_FOUND = 1,     // Item not found
    USE_RESULT_INSUFFICIENT_QUANTITY = 2, // Insufficient quantity
    USE_RESULT_CANNOT_USE = 3,         // Cannot use
    USE_RESULT_ERROR = 4               // Other error
};

// Item structure
struct Item {
    std::uint32_t item_id;
    std::string name;
    std::string description;
    int type;
    std::int32_t max_stack;
    std::int32_t price;
    std::map<std::string, std::string> attributes;
};

// Player item structure
struct PlayerItem {
    std::uint64_t item_instance_id;
    std::uint32_t item_id;
    std::int32_t quantity;
    std::uint64_t owner_id;
    std::int64_t created_time;
    std::map<std::string, std::string> attributes;
};

// Item use callback type
typedef std::function<int(const PlayerItem&, std::uint64_t)> ItemUseCallback;

// Item manager - singleton pattern
class ItemManager {
public:
    static ItemManager& Instance();
    
    // Initialize/Shutdown
    bool Initialize();
    void Shutdown();
    
    // Item template management
    bool AddItemTemplate(const Item& item);
    const Item* GetItemTemplate(std::uint32_t item_id) const;
    
    // Player item management
    bool AddItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity);
    bool RemoveItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity);
    bool RemoveItemByInstanceId(std::uint64_t owner_id, std::uint64_t item_instance_id);
    
    // Use item
    int UseItem(std::uint64_t owner_id, std::uint32_t item_id, std::int32_t quantity);
    int UseItemByInstanceId(std::uint64_t owner_id, std::uint64_t item_instance_id);
    
    // Get player items
    std::vector<PlayerItem> GetPlayerItems(std::uint64_t owner_id) const;
    std::int32_t GetItemCount(std::uint64_t owner_id, std::uint32_t item_id) const;
    
    // Item use callback registration
    void RegisterItemUseCallback(std::uint32_t item_id, ItemUseCallback callback);
    void UnregisterItemUseCallback(std::uint32_t item_id);
    
    // Generate item instance ID
    std::uint64_t GenerateItemInstanceId();
    
private:
    ItemManager();
    ~ItemManager();
    ItemManager(const ItemManager&) = delete;
    ItemManager& operator=(const ItemManager&) = delete;
    
    bool initialized_;
    mutable std::mutex mutex_;
    
    // Item templates: item_id -> Item
    std::map<std::uint32_t, Item> item_templates_;
    
    // Player items: owner_id -> vector<PlayerItem>
    std::map<std::uint64_t, std::vector<PlayerItem>> player_items_;
    
    // Item use callbacks: item_id -> callback
    std::map<std::uint32_t, ItemUseCallback> item_use_callbacks_;
    
    // Instance ID generator
    std::uint64_t next_instance_id_;
};

} // namespace item
} // namespace mmo