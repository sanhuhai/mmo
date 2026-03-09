require("socket.core")

require("LuaPanda").start("127.0.0.1", 8818)

local server = {}

server.version = "1.0.0"
server.start_time = 0

-- Load weapon manager
weapon_manager = require("weapon_manager")

function on_server_start()
    server.start_time = Utils.time_ms()
    Log.info("Lua: Server starting... version " .. server.version)
    
    -- Test weapon system
    weapon_manager.on_server_start()
    
    -- Test weapon types
    weapon_manager.test_weapon_types()
    
    Log.info("Lua: Server started successfully")
end

function on_server_stop()
    weapon_manager.on_server_stop()
    
    local uptime = Utils.time_ms() - server.start_time
    Log.info("Lua: Server stopping... uptime: " .. uptime .. "ms")
end

function on_update(delta_ms)
end

return server
