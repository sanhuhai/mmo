-- Weapon management script

function on_server_start()
    print("=== Weapon System Test ===")
    
    -- Test 1: Generate random weapons
    print("\n1. Testing random weapon generation...")
    
    -- Generate different rarity weapons
    local common_weapon = mmo.weapon.generate_random_weapon(10, mmo.weapon.WEAPON_RARITY_COMMON)
    local rare_weapon = mmo.weapon.generate_random_weapon(30, mmo.weapon.WEAPON_RARITY_RARE)
    local epic_weapon = mmo.weapon.generate_random_weapon(50, mmo.weapon.WEAPON_RARITY_EPIC)
    local legendary_weapon = mmo.weapon.generate_random_weapon(70, mmo.weapon.WEAPON_RARITY_LEGENDARY)
    
    if common_weapon then
        print("Common weapon created: " .. common_weapon:get_name())
        print(common_weapon:to_string())
    end
    
    if rare_weapon then
        print("Rare weapon created: " .. rare_weapon:get_name())
        print(rare_weapon:to_string())
    end
    
    if epic_weapon then
        print("Epic weapon created: " .. epic_weapon:get_name())
        print(epic_weapon:to_string())
    end
    
    if legendary_weapon then
        print("Legendary weapon created: " .. legendary_weapon:get_name())
        print(legendary_weapon:to_string())
    end
    
    -- Test 2: Create custom weapon
    print("\n2. Testing custom weapon creation...")
    
    local custom_weapon = mmo.weapon.create_weapon(
        12345,
        "Excalibur",
        mmo.weapon.WEAPON_TYPE_SWORD,
        mmo.weapon.WEAPON_RARITY_LEGENDARY,
        100
    )
    
    if custom_weapon then
        print("Custom weapon created: " .. custom_weapon:get_name())
        
        -- Add effects to custom weapon
        print("Adding effects to custom weapon...")
        
        local fire_effect = mmo.FireDamageEffect(50, 10)
        local ice_effect = mmo.IceSlowEffect(0.3, 5)
        local lightning_effect = mmo.LightningStrikeEffect(100, 0.2)
        
        custom_weapon:add_effect(fire_effect)
        custom_weapon:add_effect(ice_effect)
        custom_weapon:add_effect(lightning_effect)
        
        print("Updated weapon stats:")
        print(custom_weapon:to_string())
    end
    
    -- Test 3: Weapon equipment
    print("\n3. Testing weapon equipment...")
    
    local player_id = 10001
    
    if custom_weapon then
        print("Equipping weapon to player " .. player_id)
        mmo.weapon.equip_weapon(player_id, custom_weapon:get_weapon_id())
        
        local equipped_weapon = mmo.weapon.get_equipped_weapon(player_id)
        if equipped_weapon then
            print("Equipped weapon: " .. equipped_weapon:get_name())
        end
        
        print("Unequipping weapon from player " .. player_id)
        mmo.weapon.unequip_weapon(player_id)
        
        local unequipped_weapon = mmo.weapon.get_equipped_weapon(player_id)
        if not unequipped_weapon then
            print("Weapon successfully unequipped")
        end
    end
    
    -- Test 4: Weapon effects
    print("\n4. Testing weapon effects...")
    
    if custom_weapon then
        print("Applying effects to player " .. player_id)
        custom_weapon:apply_effects(player_id)
        
        print("Removing effects from player " .. player_id)
        custom_weapon:remove_effects(player_id)
    end
    
    -- Test 5: Weapon management
    print("\n5. Testing weapon management...")
    
    local weapon_count = mmo.weapon.get_weapon_count()
    print("Total weapons created: " .. weapon_count)
    
    if custom_weapon then
        local weapon_id = custom_weapon:get_weapon_id()
        print("Removing custom weapon (ID: " .. weapon_id .. ")")
        mmo.weapon.remove_weapon(weapon_id)
        
        local remaining_count = mmo.weapon.get_weapon_count()
        print("Remaining weapons: " .. remaining_count)
    end
    
    print("\n=== Weapon System Test Complete ===")
end

function on_server_stop()
    print("Weapon system shutting down...")
    local weapon_count = mmo.weapon.get_weapon_count()
    print("Weapons still in system: " .. weapon_count)
end

function create_weapon_for_player(player_id, level, rarity)
    local weapon = mmo.weapon.generate_random_weapon(level, rarity)
    if weapon then
        mmo.weapon.equip_weapon(player_id, weapon:get_weapon_id())
        return weapon
    end
    return nil
end

function get_player_weapon(player_id)
    return mmo.weapon.get_equipped_weapon(player_id)
end

function unequip_player_weapon(player_id)
    mmo.weapon.unequip_weapon(player_id)
end

function print_weapon_info(weapon)
    if weapon then
        print(weapon:to_string())
    else
        print("Weapon not found")
    end
end

-- Test function for generating weapons of different types
function test_weapon_types()
    print("\n=== Testing Weapon Types ===")
    
    local weapon_types = {
        {mmo.weapon.WEAPON_TYPE_SWORD, "Sword"},
        {mmo.weapon.WEAPON_TYPE_AXE, "Axe"},
        {mmo.weapon.WEAPON_TYPE_BOW, "Bow"},
        {mmo.weapon.WEAPON_TYPE_WAND, "Wand"},
        {mmo.weapon.WEAPON_TYPE_DAGGER, "Dagger"},
        {mmo.weapon.WEAPON_TYPE_SHIELD, "Shield"},
        {mmo.weapon.WEAPON_TYPE_GUN, "Gun"},
        {mmo.weapon.WEAPON_TYPE_STAFF, "Staff"},
        {mmo.weapon.WEAPON_TYPE_SPEAR, "Spear"},
        {mmo.weapon.WEAPON_TYPE_FIST, "Fist"}
    }
    
    for _, weapon_type in ipairs(weapon_types) do
        local type_id, type_name = unpack(weapon_type)
        local weapon = mmo.weapon.create_weapon(
            math.random(10000, 99999),
            "Test " .. type_name,
            type_id,
            mmo.weapon.WEAPON_RARITY_RARE,
            30
        )
        if weapon then
            print("Created " .. type_name .. ": " .. weapon:get_name())
            print("  Attack: " .. weapon:get_attribute().attack)
            print("  Defense: " .. weapon:get_attribute().defense)
            print("  Magic Power: " .. weapon:get_attribute().magic_power)
        end
    end
end

-- Export functions for use in other scripts
return {
    create_weapon_for_player = create_weapon_for_player,
    get_player_weapon = get_player_weapon,
    unequip_player_weapon = unequip_player_weapon,
    print_weapon_info = print_weapon_info,
    test_weapon_types = test_weapon_types
}
