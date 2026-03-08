#include "script/lua_register.h"
#include "player/weapon.h"

// Forward declaration for protobuf Lua registration
extern void RegisterProtoToLua(lua_State* L);

namespace mmo {

void LuaRegister::RegisterService(LuaEngine& engine) {
    // Register protobuf messages to Lua
    #ifdef USE_PROTOBUF
        RegisterProtoToLua(engine.GetState());
    #endif
    
    auto L = engine.GetState();
    
    // Register weapon system
    luabridge::getGlobalNamespace(L)
        .beginNamespace("mmo")
            .beginNamespace("weapon")
                // WeaponType enum
                .addVariable("WEAPON_TYPE_SWORD", static_cast<int>(0)) // SWORD
                .addVariable("WEAPON_TYPE_AXE", static_cast<int>(1)) // AXE
                .addVariable("WEAPON_TYPE_BOW", static_cast<int>(2)) // BOW
                .addVariable("WEAPON_TYPE_WAND", static_cast<int>(3)) // WAND
                .addVariable("WEAPON_TYPE_DAGGER", static_cast<int>(4)) // DAGGER
                .addVariable("WEAPON_TYPE_SHIELD", static_cast<int>(5)) // SHIELD
                .addVariable("WEAPON_TYPE_GUN", static_cast<int>(6)) // GUN
                .addVariable("WEAPON_TYPE_STAFF", static_cast<int>(7)) // STAFF
                .addVariable("WEAPON_TYPE_SPEAR", static_cast<int>(8)) // SPEAR
                .addVariable("WEAPON_TYPE_FIST", static_cast<int>(9)) // FIST
                
                // WeaponRarity enum
                .addVariable("WEAPON_RARITY_COMMON", static_cast<int>(0)) // COMMON
                .addVariable("WEAPON_RARITY_UNCOMMON", static_cast<int>(1)) // UNCOMMON
                .addVariable("WEAPON_RARITY_RARE", static_cast<int>(2)) // RARE
                .addVariable("WEAPON_RARITY_EPIC", static_cast<int>(3)) // EPIC
                .addVariable("WEAPON_RARITY_LEGENDARY", static_cast<int>(4)) // LEGENDARY
                
                // WeaponManager functions
                .addFunction("create_weapon", [](uint64_t weapon_id, const std::string& name, int type, int rarity, int level) {
                    return mmo::WeaponManager::Instance().CreateWeapon(
                        weapon_id, 
                        name, 
                        static_cast<mmo::WeaponType>(type), 
                        static_cast<mmo::WeaponRarity>(rarity), 
                        level
                    );
                })
                .addFunction("get_weapon", [](uint64_t weapon_id) {
                    return mmo::WeaponManager::Instance().GetWeapon(weapon_id);
                })
                .addFunction("remove_weapon", [](uint64_t weapon_id) {
                    mmo::WeaponManager::Instance().RemoveWeapon(weapon_id);
                })
                .addFunction("generate_random_weapon", [](int level, int rarity) {
                    return mmo::WeaponManager::Instance().GenerateRandomWeapon(
                        level, 
                        static_cast<mmo::WeaponRarity>(rarity)
                    );
                })
                .addFunction("equip_weapon", [](uint64_t player_id, uint64_t weapon_id) {
                    return mmo::WeaponManager::Instance().EquipWeapon(player_id, weapon_id);
                })
                .addFunction("unequip_weapon", [](uint64_t player_id) {
                    return mmo::WeaponManager::Instance().UnequipWeapon(player_id);
                })
                .addFunction("get_equipped_weapon", [](uint64_t player_id) {
                    return mmo::WeaponManager::Instance().GetEquippedWeapon(player_id);
                })
                .addFunction("get_weapon_count", []() {
                    return mmo::WeaponManager::Instance().GetWeaponCount();
                })
            .endNamespace()
        .endNamespace();
    
    // Register Weapon class
    luabridge::getGlobalNamespace(L)
        .beginClass<mmo::Weapon>("Weapon")
            .addFunction("get_weapon_id", &mmo::Weapon::GetWeaponId)
            .addFunction("get_name", &mmo::Weapon::GetName)
            .addFunction("get_type", &mmo::Weapon::GetType)
            .addFunction("get_rarity", &mmo::Weapon::GetRarity)
            .addFunction("get_level", &mmo::Weapon::GetLevel)
            .addFunction("add_effect", &mmo::Weapon::AddEffect)
            .addFunction("remove_effect", &mmo::Weapon::RemoveEffect)
            .addFunction("apply_effects", &mmo::Weapon::ApplyEffects)
            .addFunction("remove_effects", &mmo::Weapon::RemoveEffects)
            .addFunction("to_string", &mmo::Weapon::ToString)
        .endClass();
    
    // Register WeaponEffect class
    luabridge::getGlobalNamespace(L)
        .beginClass<mmo::WeaponEffect>("WeaponEffect")
            .addFunction("get_name", &mmo::WeaponEffect::GetName)
            .addFunction("get_description", &mmo::WeaponEffect::GetDescription)
            .addFunction("is_active", &mmo::WeaponEffect::IsActive)
        .endClass();
    
    // Register FireDamageEffect class
    luabridge::getGlobalNamespace(L)
        .deriveClass<mmo::FireDamageEffect, mmo::WeaponEffect>("FireDamageEffect")
            .addConstructor<void(*)(int, int)>()
        .endClass();
    
    // Register IceSlowEffect class
    luabridge::getGlobalNamespace(L)
        .deriveClass<mmo::IceSlowEffect, mmo::WeaponEffect>("IceSlowEffect")
            .addConstructor<void(*)(float, int)>()
        .endClass();
    
    // Register LightningStrikeEffect class
    luabridge::getGlobalNamespace(L)
        .deriveClass<mmo::LightningStrikeEffect, mmo::WeaponEffect>("LightningStrikeEffect")
            .addConstructor<void(*)(int, float)>()
        .endClass();
}

} 
