local server = {}

server.version = "1.0.0"
server.start_time = 0

-- Load weapon manager
weapon_manager = require("weapon_manager")

function on_server_start()
    server.start_time = mmo.utils.time_ms()
    mmo.log_info("Lua: Server starting... version " .. server.version)
    
    -- Test weapon system
    weapon_manager.on_server_start()
    
    -- Test weapon types
    weapon_manager.test_weapon_types()
    
    mmo.log_info("Lua: Server started successfully")
end

function on_server_stop()
    weapon_manager.on_server_stop()
    
    local uptime = mmo.utils.time_ms() - server.start_time
    mmo.log_info("Lua: Server stopping... uptime: " .. uptime .. "ms")
end

function on_update(delta_ms)
end

return server
