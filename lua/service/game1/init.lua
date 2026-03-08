local game = {}

game.player_count = 0
game.players = {}

function on_service_message(source, type, data)
    mmo.log_debug("Lua Game: Service message from " .. source .. " type: " .. type)
    
    if type == 1 then
        local player_id = tonumber(data)
        local player = game.get_player(player_id)
        if player then
            mmo.log_info("Lua Game: Player " .. player.name .. " info: HP=" .. player.hp .. " MP=" .. player.mp)
        end
    end
end

function on_update(delta_ms)
end

function game.create_test_player(player_id, conn_id, name)
    if game.create_player(player_id, conn_id, name) then
        game.player_count = game.player_count + 1
        mmo.log_info("Lua Game: Created player " .. name .. " (ID: " .. player_id .. ")")
        return true
    end
    return false
end

function game.get_player_info(player_id)
    local player = game.get_player(player_id)
    if player then
        return {
            id = player.player_id,
            name = player.name,
            level = player.level,
            hp = player.hp,
            mp = player.mp,
            x = player.x,
            y = player.y,
            z = player.z
        }
    end
    return nil
end

return game
