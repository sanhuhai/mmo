-- Example script to demonstrate player data loading from database and syncing to Lua

-- Load player manager module
local player_manager = require("player_manager")

print("=== Player Database Example ===")
print()

-- Load all players from database
print("Step 1: Loading all players from database...")
player_manager.load_all_players()
print()

-- Sync all players to Lua
print("Step 2: Syncing all players to Lua...")
player_manager.sync_all_players()
print()

-- Display all players
print("Step 3: Displaying all players...")
player_manager.display_all_players()
print()

-- Display specific player info
print("Step 4: Displaying player 1 info...")
player_manager.display_player_info(1)
print()

-- Display specific player info
print("Step 5: Displaying player 50 info...")
player_manager.display_player_info(50)
print()

-- Display specific player info
print("Step 6: Displaying player 100 info...")
player_manager.display_player_info(100)
print()

print("=== Example completed ===")
