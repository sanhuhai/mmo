local example = {}

function example.run()
    mmo.log_info("=== MMO Server Lua Example ===")
    
    mmo.log_info("1. Testing config...")
    local server_name = config:get("server.name", "Default Server")
    mmo.log_info("   Server name: " .. server_name:as_string())
    
    mmo.log_info("2. Testing utils...")
    local time_ms = mmo.utils.time_ms()
    local time_sec = mmo.utils.time_sec()
    mmo.log_info("   Time (ms): " .. time_ms)
    mmo.log_info("   Time (sec): " .. time_sec)
    
    mmo.log_info("3. Testing service manager...")
    local service_count = ServiceManager.get_service_count()
    mmo.log_info("   Service count: " .. service_count)
    
    mmo.log_info("=== Example Complete ===")
end

return example
