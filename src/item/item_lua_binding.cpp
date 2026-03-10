#include "item/item_lua_binding.h"
#include <cstdint>
#include <string>
#include <LuaBridge/LuaBridge.h>

namespace mmo {
namespace item {

void ItemLuaBinding::Register(LuaEngine& engine) {
    RegisterItemTypeConstants(engine);
    RegisterUseResultConstants(engine);
    RegisterItem(engine);
    RegisterPlayerItem(engine);
    RegisterItemManager(engine);
}

void ItemLuaBinding::RegisterItemTypeConstants(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Item")
            .beginNamespace("Type")
                .addVariable("CONSUMABLE", ITEM_TYPE_CONSUMABLE)
                .addVariable("EQUIPMENT", ITEM_TYPE_EQUIPMENT)
                .addVariable("MATERIAL", ITEM_TYPE_MATERIAL)
                .addVariable("CURRENCY", ITEM_TYPE_CURRENCY)
                .addVariable("QUEST", ITEM_TYPE_QUEST)
                .addVariable("SPECIAL", ITEM_TYPE_SPECIAL)
            .endNamespace()
        .endNamespace();
}

void ItemLuaBinding::RegisterUseResultConstants(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Item")
            .beginNamespace("UseResult")
                .addVariable("SUCCESS", USE_RESULT_SUCCESS)
                .addVariable("ITEM_NOT_FOUND", USE_RESULT_ITEM_NOT_FOUND)
                .addVariable("INSUFFICIENT_QUANTITY", USE_RESULT_INSUFFICIENT_QUANTITY)
                .addVariable("CANNOT_USE", USE_RESULT_CANNOT_USE)
                .addVariable("ERROR", USE_RESULT_ERROR)
            .endNamespace()
        .endNamespace();
}

void ItemLuaBinding::RegisterItem(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Item")
            .beginClass<Item>("Item")
                .addConstructor<void(*)()>()
                .addProperty("item_id", 
                    [](const Item* item) { return item->item_id; },
                    [](Item* item, std::uint32_t val) { item->item_id = val; })
                .addProperty("name", 
                    [](const Item* item) { return item->name; },
                    [](Item* item, const std::string& val) { item->name = val; })
                .addProperty("description", 
                    [](const Item* item) { return item->description; },
                    [](Item* item, const std::string& val) { item->description = val; })
                .addProperty("type", 
                    [](const Item* item) { return item->type; },
                    [](Item* item, int val) { item->type = val; })
                .addProperty("max_stack", 
                    [](const Item* item) { return item->max_stack; },
                    [](Item* item, std::int32_t val) { item->max_stack = val; })
                .addProperty("price", 
                    [](const Item* item) { return item->price; },
                    [](Item* item, std::int32_t val) { item->price = val; })
            .endClass()
        .endNamespace();
}

void ItemLuaBinding::RegisterPlayerItem(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Item")
            .beginClass<PlayerItem>("PlayerItem")
                .addConstructor<void(*)()>()
                .addProperty("item_instance_id", 
                    [](const PlayerItem* item) { return item->item_instance_id; },
                    [](PlayerItem* item, std::uint64_t val) { item->item_instance_id = val; })
                .addProperty("item_id", 
                    [](const PlayerItem* item) { return item->item_id; },
                    [](PlayerItem* item, std::uint32_t val) { item->item_id = val; })
                .addProperty("quantity", 
                    [](const PlayerItem* item) { return item->quantity; },
                    [](PlayerItem* item, std::int32_t val) { item->quantity = val; })
                .addProperty("owner_id", 
                    [](const PlayerItem* item) { return item->owner_id; },
                    [](PlayerItem* item, std::uint64_t val) { item->owner_id = val; })
                .addProperty("created_time", 
                    [](const PlayerItem* item) { return item->created_time; },
                    [](PlayerItem* item, std::int64_t val) { item->created_time = val; })
            .endClass()
        .endNamespace();
}

void ItemLuaBinding::RegisterItemManager(LuaEngine& engine) {
    luabridge::getGlobalNamespace(engine.GetState())
        .beginNamespace("Item")
            .beginClass<ItemManager>("ItemManager")
                .addStaticFunction("instance", &ItemManager::Instance)
                .addFunction("initialize", &ItemManager::Initialize)
                .addFunction("shutdown", &ItemManager::Shutdown)
                .addFunction("add_item", &ItemManager::AddItem)
                .addFunction("remove_item", &ItemManager::RemoveItem)
                .addFunction("remove_item_by_instance_id", &ItemManager::RemoveItemByInstanceId)
                .addFunction("use_item", &ItemManager::UseItem)
                .addFunction("use_item_by_instance_id", &ItemManager::UseItemByInstanceId)
                .addFunction("get_item_count", &ItemManager::GetItemCount)
            .endClass()
        .endNamespace();
}

} // namespace item
} // namespace mmo