-- Skynet main entry point for MMO Server integration
-- This script is loaded when skynet node starts

local skynet = require "skynet"
local service = require "skynet.service"

skynet.start(function()
    skynet.error("Skynet node started with MMO integration")
    
    -- Load MMO configuration
    local mmo_config = {
        forward_to_mmo = true,
        service_dir = "./lua/service"
    }
    
    -- Start MMO bridge service
    -- This service acts as a bridge between skynet and MMO framework
    local bridge = skynet.newservice("mmo_bridge")
    skynet.call(bridge, "lua", "init", mmo_config)
    
    -- Register with MMO ServiceManager
    -- This allows MMO services to communicate with skynet services
    skynet.send(bridge, "lua", "register_with_mmo")
    
    -- Load Lua services from mmo/service directory
    local service_dir = mmo_config.service_dir
    skynet.error("Loading MMO Lua services from: " .. service_dir)
    
    -- Example: Start a simple echo service
    -- local echo_service = skynet.newservice("echo")
    
    -- Example: Start gate service
    -- local gate = skynet.newservice("gate")
    
    skynet.error("MMO Skynet integration ready")
end)
