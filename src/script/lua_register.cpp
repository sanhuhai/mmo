#include "script/lua_register.h"
#include "player/weapon.h"

#ifdef USE_MYSQL
#include "player/player_manager.h"
#endif

extern void RegisterProtoToLua(lua_State* L);

namespace mmo {

void LuaRegister::RegisterService(LuaEngine& engine) {
    #ifdef USE_PROTOBUF
        RegisterProtoToLua(engine.GetState());
    #endif
    
    auto L = engine.GetState();
    
    // 注册Weapon模块
    luabridge::getGlobalNamespace(L)
        .beginNamespace("Weapon")
            // WeaponType枚举
            .addVariable("TYPE_SWORD", static_cast<int>(WeaponType::SWORD))
            .addVariable("TYPE_AXE", static_cast<int>(WeaponType::AXE))
            .addVariable("TYPE_BOW", static_cast<int>(WeaponType::BOW))
            .addVariable("TYPE_WAND", static_cast<int>(WeaponType::WAND))
            .addVariable("TYPE_DAGGER", static_cast<int>(WeaponType::DAGGER))
            .addVariable("TYPE_SHIELD", static_cast<int>(WeaponType::SHIELD))
            .addVariable("TYPE_GUN", static_cast<int>(WeaponType::GUN))
            .addVariable("TYPE_STAFF", static_cast<int>(WeaponType::STAFF))
            .addVariable("TYPE_SPEAR", static_cast<int>(WeaponType::SPEAR))
            .addVariable("TYPE_FIST", static_cast<int>(WeaponType::FIST))
            
            // WeaponRarity枚举
            .addVariable("RARITY_COMMON", static_cast<int>(WeaponRarity::COMMON))
            .addVariable("RARITY_UNCOMMON", static_cast<int>(WeaponRarity::UNCOMMON))
            .addVariable("RARITY_RARE", static_cast<int>(WeaponRarity::RARE))
            .addVariable("RARITY_EPIC", static_cast<int>(WeaponRarity::EPIC))
            .addVariable("RARITY_LEGENDARY", static_cast<int>(WeaponRarity::LEGENDARY))
            
            // WeaponManager方法
            .addFunction("create", [](uint64_t weapon_id, const std::string& name, int type, int rarity, int level) {
                return WeaponManager::Instance().CreateWeapon(
                    weapon_id, 
                    name, 
                    static_cast<WeaponType>(type), 
                    static_cast<WeaponRarity>(rarity), 
                    level
                );
            })
            .addFunction("get", [](uint64_t weapon_id) {
                return WeaponManager::Instance().GetWeapon(weapon_id);
            })
            .addFunction("remove", [](uint64_t weapon_id) {
                WeaponManager::Instance().RemoveWeapon(weapon_id);
            })
            .addFunction("generate_random", [](int level, int rarity) {
                return WeaponManager::Instance().GenerateRandomWeapon(
                    level, 
                    static_cast<WeaponRarity>(rarity)
                );
            })
            .addFunction("equip", [](uint64_t player_id, uint64_t weapon_id) {
                return WeaponManager::Instance().EquipWeapon(player_id, weapon_id);
            })
            .addFunction("unequip", [](uint64_t player_id) {
                return WeaponManager::Instance().UnequipWeapon(player_id);
            })
            .addFunction("get_equipped", [](uint64_t player_id) {
                return WeaponManager::Instance().GetEquippedWeapon(player_id);
            })
            .addFunction("count", []() {
                return WeaponManager::Instance().GetWeaponCount();
            })
        .endNamespace();
    
    #ifdef USE_MYSQL
        // 注册Player模块
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Player")
                .addFunction("load_all_from_db", []() {
                    return PlayerManager::Instance().LoadAllPlayersFromDatabase();
                })
                .addFunction("load_from_db", [](uint64_t player_id) {
                    return PlayerManager::Instance().LoadPlayerFromDatabase(player_id);
                })
                .addFunction("save_to_db", [](uint64_t player_id) {
                    return PlayerManager::Instance().SavePlayerToDatabase(player_id);
                })
                .addFunction("sync_all_to_lua", [L]() {
                    return PlayerManager::Instance().SyncAllPlayersToLua(L);
                })
                .addFunction("sync_to_lua", [L](uint64_t player_id) {
                    return PlayerManager::Instance().SyncPlayerToLua(L, player_id);
                })
                .addFunction("count", []() {
                    return PlayerManager::Instance().GetPlayerCount();
                })
                .addFunction("remove_from_cache", [](uint64_t player_id) {
                    return PlayerManager::Instance().RemovePlayerFromCache(player_id);
                })
            .endNamespace();
    #endif
    
    // 注册Weapon类（供Lua创建实例）
    luabridge::getGlobalNamespace(L)
        .beginClass<Weapon>("WeaponInstance")
            .addFunction("get_id", &Weapon::GetWeaponId)
            .addFunction("get_name", &Weapon::GetName)
            .addFunction("get_type", &Weapon::GetType)
            .addFunction("get_rarity", &Weapon::GetRarity)
            .addFunction("get_level", &Weapon::GetLevel)
            .addFunction("add_effect", &Weapon::AddEffect)
            .addFunction("remove_effect", &Weapon::RemoveEffect)
            .addFunction("apply_effects", &Weapon::ApplyEffects)
            .addFunction("remove_effects", &Weapon::RemoveEffects)
            .addFunction("to_string", &Weapon::ToString)
        .endClass();
    
    // 注册WeaponEffect类
    luabridge::getGlobalNamespace(L)
        .beginClass<WeaponEffect>("WeaponEffect")
            .addFunction("get_name", &WeaponEffect::GetName)
            .addFunction("get_description", &WeaponEffect::GetDescription)
            .addFunction("is_active", &WeaponEffect::IsActive)
        .endClass();
    
    // 注册FireDamageEffect类
    luabridge::getGlobalNamespace(L)
        .deriveClass<FireDamageEffect, WeaponEffect>("FireDamageEffect")
            .addConstructor<void(*)(int, int)>()
        .endClass();
    
    // 注册IceSlowEffect类
    luabridge::getGlobalNamespace(L)
        .deriveClass<IceSlowEffect, WeaponEffect>("IceSlowEffect")
            .addConstructor<void(*)(float, int)>()
        .endClass();
    
    // 注册LightningStrikeEffect类
    luabridge::getGlobalNamespace(L)
        .deriveClass<LightningStrikeEffect, WeaponEffect>("LightningStrikeEffect")
            .addConstructor<void(*)(int, float)>()
        .endClass();
}

}