-- Player Manager Lua Module
-- This module provides functions to manage player data from database

local player_manager = {}

-- Initialize player manager
function player_manager.initialize()
    print("Player Manager initialized")
end

-- Load all players from database
function player_manager.load_all_players()
    print("Loading all players from database...")
    local success = mmo.player.load_all_players_from_database()
    if success then
        print("Successfully loaded all players from database")
        local count = mmo.player.get_player_count()
        print("Total players: " .. tostring(count))
    else
        print("Failed to load players from database")
    end
    return success
end

-- Load a specific player from database
function player_manager.load_player(player_id)
    print("Loading player " .. tostring(player_id) .. " from database...")
    local success = mmo.player.load_player_from_database(player_id)
    if success then
        print("Successfully loaded player " .. tostring(player_id))
    else
        print("Failed to load player " .. tostring(player_id))
    end
    return success
end

-- Save a player to database
function player_manager.save_player(player_id)
    print("Saving player " .. tostring(player_id) .. " to database...")
    local success = mmo.player.save_player_to_database(player_id)
    if success then
        print("Successfully saved player " .. tostring(player_id))
    else
        print("Failed to save player " .. tostring(player_id))
    end
    return success
end

-- Sync all players to Lua
function player_manager.sync_all_players()
    print("Syncing all players to Lua...")
    local success = mmo.player.sync_all_players_to_lua()
    if success then
        print("Successfully synced all players to Lua")
    else
        print("Failed to sync players to Lua")
    end
    return success
end

-- Sync a specific player to Lua
function player_manager.sync_player(player_id)
    print("Syncing player " .. tostring(player_id) .. " to Lua...")
    local success = mmo.player.sync_player_to_lua(player_id)
    if success then
        print("Successfully synced player " .. tostring(player_id) .. " to Lua")
    else
        print("Failed to sync player " .. tostring(player_id) .. " to Lua")
    end
    return success
end

-- Get player count
function player_manager.get_count()
    local count = mmo.player.get_player_count()
    print("Total players in cache: " .. tostring(count))
    return count
end

-- Remove player from cache
function player_manager.remove_player(player_id)
    print("Removing player " .. tostring(player_id) .. " from cache...")
    local success = mmo.player.remove_player_from_cache(player_id)
    if success then
        print("Successfully removed player " .. tostring(player_id))
    else
        print("Failed to remove player " .. tostring(player_id))
    end
    return success
end

-- Get player info from cache
function player_manager.get_player_info(player_id)
    if mmo.player_cache and mmo.player_cache[tostring(player_id)] then
        return mmo.player_cache[tostring(player_id)]
    else
        print("Player " .. tostring(player_id) .. " not found in cache")
        return nil
    end
end

-- Display player info
function player_manager.display_player_info(player_id)
    local player_info = player_manager.get_player_info(player_id)
    if player_info then
        print("=== Player Info ===")
        print("Player ID: " .. tostring(player_info.player_id))
        print("Name: " .. player_info.name)
        print("Level: " .. tostring(player_info.level))
        print("Experience: " .. tostring(player_info.exp))
        print("HP: " .. tostring(player_info.hp) .. "/" .. tostring(player_info.max_hp))
        print("MP: " .. tostring(player_info.mp) .. "/" .. tostring(player_info.max_mp))
        print("Position: (" .. tostring(player_info.position.x) .. ", " .. tostring(player_info.position.y) .. ", " .. tostring(player_info.position.z) .. ")")
        print("Rotation: (" .. tostring(player_info.rotation.x) .. ", " .. tostring(player_info.rotation.y) .. ", " .. tostring(player_info.rotation.z) .. ")")
        print("Profession: " .. tostring(player_info.profession))
        print("Gender: " .. tostring(player_info.gender))
        print("==================")
    end
end

-- Display all players
function player_manager.display_all_players()
    print("=== All Players ===")
    if mmo.player_cache then
        local count = 0
        for player_id, player_info in pairs(mmo.player_cache) do
            print("[" .. player_id .. "] " .. player_info.name .. " (Level " .. tostring(player_info.level) .. ")")
            count = count + 1
        end
        print("Total: " .. tostring(count) .. " players")
    else
        print("No players in cache")
    end
    print("==================")
end

-- Initialize the module
player_manager.initialize()

return player_manager
