function on_server_start()
    print("Server started!")
    
    if mmo.player_cache then
        print("Player cache module loaded")
    end
end

function on_update(delta_ms)
    -- Update loop called every frame
end

function on_server_stop()
    print("Server stopped!")
end

function on_player_login(player_id, player_data)
    print("Player logged in: " .. player_id)
    
    if player_data then
        print("Player name: " .. (player_data.name or "Unknown"))
        print("Player level: " .. (player_data.level or 1))
        print("Player HP: " .. (player_data.hp or 100))
        print("Player MP: " .. (player_data.mp or 100))
        print("Player position: (" .. 
               (player_data.position and player_data.position.x or 0) .. ", " ..
               (player_data.position and player_data.position.y or 0) .. ", " ..
               (player_data.position and player_data.position.z or 0) .. ")")
    end
end

function on_player_logout(player_id)
    print("Player logged out: " .. player_id)
end

function on_player_move(player_id, position)
    print("Player " .. player_id .. " moved to: (" .. 
           (position.x or 0) .. ", " .. 
           (position.y or 0) .. ", " .. 
           (position.z or 0) .. ")")
end

function on_player_hp_changed(player_id, hp, max_hp)
    print("Player " .. player_id .. " HP changed: " .. hp .. "/" .. max_hp)
end

function on_player_mp_changed(player_id, mp, max_mp)
    print("Player " .. player_id .. " MP changed: " .. mp .. "/" .. max_mp)
end

function get_player_from_cache(player_id)
    if mmo.player_cache then
        return mmo.player_cache[player_id]
    end
    return nil
end

function update_player_position(player_id, x, y, z)
    print("Updating player " .. player_id .. " position to: (" .. x .. ", " .. y .. ", " .. z .. ")")
    
    local player_data = get_player_from_cache(player_id)
    if player_data then
        player_data.position = {
            x = x,
            y = y,
            z = z
        }
    end
end

function save_player_data(player_id)
    print("Saving player data for: " .. player_id)
    
    local player_data = get_player_from_cache(player_id)
    if player_data then
        print("Player data to save:")
        print("  Name: " .. (player_data.name or "Unknown"))
        print("  Level: " .. (player_data.level or 1))
        print("  HP: " .. (player_data.hp or 100))
        print("  MP: " .. (player_data.mp or 100))
        print("  Position: (" .. 
               (player_data.position and player_data.position.x or 0) .. ", " ..
               (player_data.position and player_data.position.y or 0) .. ", " ..
               (player_data.position and player_data.position.z or 0) .. ")")
    end
end
