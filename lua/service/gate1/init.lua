local gate = {}

gate.client_count = 0
gate.clients = {}
gate.proto_handler = require("service.gate1.proto_handler")

function on_client_connect(conn_id, address)
    gate.client_count = gate.client_count + 1
    gate.clients[conn_id] = {
        id = conn_id,
        address = address,
        connect_time = mmo.utils.time_ms(),
        player_id = 0,
        authenticated = false
    }
    
    mmo.log_info("Lua Gate: Client " .. conn_id .. " connected from " .. address)
    
    -- 发送欢迎消息
    local welcome = {
        result = {
            code = 0,
            message = "Welcome to MMO Server!"
        },
        server_time = tostring(mmo.utils.time_sec())
    }
    
    local welcome_data = protobuf.encode("mmo.Result", welcome.result)
    gate.send_proto(conn_id, mmo.msg.MSG_ERROR, welcome_data)
end

function on_client_message(conn_id, message)
    -- 解析GameMessage
    local game_msg = protobuf.decode("mmo.GameMessage", message)
    if not game_msg then
        mmo.log_error("Failed to parse GameMessage from " .. conn_id)
        return
    end
    
    mmo.log_debug("Lua Gate: Received message type " .. game_msg.type .. " from " .. conn_id)
    
    -- 分发到对应的处理器
    gate.proto_handler.handle_message(conn_id, game_msg.type, game_msg.payload)
end

function on_client_disconnect(conn_id)
    local client = gate.clients[conn_id]
    if client then
        mmo.log_info("Lua Gate: Client " .. conn_id .. " disconnected, player_id: " .. client.player_id)
    end
    
    gate.client_count = gate.client_count - 1
    gate.clients[conn_id] = nil
end

function on_service_message(source, type, data)
    mmo.log_debug("Lua Gate: Service message from " .. source .. " type: " .. type)
end

function on_update(delta_ms)
end

-- Proto发送函数
gate.send_proto = function(conn_id, msg_type, payload)
    local game_msg = {
        type = msg_type,
        sequence = 0,
        timestamp = mmo.utils.time_sec(),
        payload = payload
    }
    
    local data = protobuf.encode("mmo.GameMessage", game_msg)
    gate.send_to_client(conn_id, data)
end

gate.broadcast_proto = function(msg_type, payload)
    local game_msg = {
        type = msg_type,
        sequence = 0,
        timestamp = mmo.utils.time_sec(),
        payload = payload
    }
    
    local data = protobuf.encode("mmo.GameMessage", game_msg)
    gate.broadcast(data)
end

-- 初始化protobuf
protobuf.set_path("proto")
protobuf.import("common.proto")
protobuf.import("login.proto")
protobuf.import("player.proto")
protobuf.import("chat.proto")
protobuf.import("game.proto")

mmo.log_info("Lua Gate: Proto files loaded")

return gate
