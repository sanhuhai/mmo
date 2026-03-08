local protobuf_example = {}

function protobuf_example.test()
    mmo.log_info("=== Protobuf Example ===")
    
    -- 设置proto文件路径
    protobuf.set_path("proto")
    
    -- 导入proto文件
    protobuf.import("common.proto")
    protobuf.import("login.proto")
    protobuf.import("player.proto")
    protobuf.import("chat.proto")
    
    mmo.log_info("1. Testing Vector3 encoding/decoding...")
    local vector3_data = {
        x = 100.5,
        y = 200.3,
        z = 50.0
    }
    local encoded = protobuf.encode("mmo.Vector3", vector3_data)
    mmo.log_info("   Encoded Vector3 size: " .. #encoded)
    
    local decoded = protobuf.decode("mmo.Vector3", encoded)
    mmo.log_info("   Decoded Vector3: x=" .. decoded.x .. ", y=" .. decoded.y .. ", z=" .. decoded.z)
    
    mmo.log_info("2. Testing LoginRequest encoding/decoding...")
    local login_request = {
        account = "test_user",
        password = "test_pass",
        platform = "pc",
        version = "1.0.0",
        device_id = "device_12345"
    }
    local login_encoded = protobuf.encode("mmo.LoginRequest", login_request)
    mmo.log_info("   Encoded LoginRequest size: " .. #login_encoded)
    
    local login_decoded = protobuf.decode("mmo.LoginRequest", login_encoded)
    mmo.log_info("   Decoded LoginRequest: account=" .. login_decoded.account .. 
                 ", platform=" .. login_decoded.platform)
    
    mmo.log_info("3. Testing PlayerInfo encoding/decoding...")
    local player_info = {
        player_id = 10001,
        name = "Hero",
        level = 10,
        exp = 5000,
        hp = 100,
        max_hp = 100,
        mp = 50,
        max_mp = 50,
        position = {
            x = 100.0,
            y = 0.0,
            z = 200.0
        },
        rotation = {
            x = 0.0,
            y = 90.0,
            z = 0.0
        },
        profession = 1,
        gender = 1
    }
    local player_encoded = protobuf.encode("mmo.PlayerInfo", player_info)
    mmo.log_info("   Encoded PlayerInfo size: " .. #player_encoded)
    
    local player_decoded = protobuf.decode("mmo.PlayerInfo", player_encoded)
    mmo.log_info("   Decoded PlayerInfo: name=" .. player_decoded.name .. 
                 ", level=" .. player_decoded.level .. 
                 ", pos=(" .. player_decoded.position.x .. ", " .. player_decoded.position.y .. ", " .. player_decoded.position.z .. ")")
    
    mmo.log_info("4. Testing ChatMessage encoding/decoding...")
    local chat_message = {
        sender_id = 10001,
        sender_name = "Hero",
        channel = 0,  -- CHANNEL_WORLD
        content = "Hello, World!",
        timestamp = mmo.utils.time_sec(),
        target_id = 0
    }
    local chat_encoded = protobuf.encode("mmo.ChatMessage", chat_message)
    mmo.log_info("   Encoded ChatMessage size: " .. #chat_encoded)
    
    local chat_decoded = protobuf.decode("mmo.ChatMessage", chat_encoded)
    mmo.log_info("   Decoded ChatMessage: " .. chat_decoded.sender_name .. ": " .. chat_decoded.content)
    
    mmo.log_info("=== Protobuf Example Complete ===")
    
    return true
end

return protobuf_example
