local proto_handler = {}

proto_handler.handlers = {}

function proto_handler.register(msg_type, handler)
    proto_handler.handlers[msg_type] = handler
    mmo.log_info("Registered handler for message type: " .. msg_type)
end

function proto_handler.handle_message(conn_id, msg_type, msg_data)
    local handler = proto_handler.handlers[msg_type]
    if handler then
        local success, result = pcall(handler, conn_id, msg_data)
        if not success then
            mmo.log_error("Error handling message type " .. msg_type .. ": " .. tostring(result))
        end
    else
        mmo.log_warn("No handler for message type: " .. msg_type)
    end
end

-- 注册登录请求处理器
proto_handler.register(mmo.msg.MSG_LOGIN_REQUEST, function(conn_id, data)
    local login_req = protobuf.decode("mmo.LoginRequest", data)
    mmo.log_info("Login request: account=" .. login_req.account .. ", platform=" .. login_req.platform)
    
    -- 验证登录
    local success = true
    local player_id = 10001
    local token = "token_" .. tostring(player_id)
    
    -- 发送登录响应
    local login_resp = {
        result = {
            code = success and 0 or 1,
            message = success and "Success" or "Invalid credentials"
        },
        player_id = player_id,
        token = token,
        server_time = tostring(mmo.utils.time_sec())
    }
    
    local resp_data = protobuf.encode("mmo.LoginResponse", login_resp)
    gate.send_proto(conn_id, mmo.msg.MSG_LOGIN_RESPONSE, resp_data)
    
    mmo.log_info("Login response sent to " .. conn_id)
end)

-- 注册玩家移动请求处理器
proto_handler.register(mmo.msg.MSG_PLAYER_MOVE_REQUEST, function(conn_id, data)
    local move_req = protobuf.decode("mmo.PlayerMoveRequest", data)
    mmo.log_debug("Player move request from " .. conn_id .. 
                  " to (" .. move_req.target_position.x .. ", " .. 
                  move_req.target_position.y .. ", " .. 
                  move_req.target_position.z .. ")")
    
    -- 处理移动逻辑
    local move_resp = {
        result = {
            code = 0,
            message = "Success"
        },
        position = move_req.target_position
    }
    
    local resp_data = protobuf.encode("mmo.PlayerMoveResponse", move_resp)
    gate.send_proto(conn_id, mmo.msg.MSG_PLAYER_MOVE_RESPONSE, resp_data)
end)

-- 注册聊天请求处理器
proto_handler.register(mmo.msg.MSG_SEND_CHAT_REQUEST, function(conn_id, data)
    local chat_req = protobuf.decode("mmo.SendChatRequest", data)
    mmo.log_info("Chat message: channel=" .. chat_req.channel .. ", content=" .. chat_req.content)
    
    -- 广播聊天消息
    local chat_notify = {
        message = {
            sender_id = conn_id,
            sender_name = "Player" .. conn_id,
            channel = chat_req.channel,
            content = chat_req.content,
            timestamp = mmo.utils.time_sec(),
            target_id = chat_req.target_id
        }
    }
    
    local notify_data = protobuf.encode("mmo.ReceiveChatNotify", chat_notify)
    gate.broadcast_proto(mmo.msg.MSG_RECEIVE_CHAT, notify_data)
    
    -- 发送响应
    local chat_resp = {
        result = {
            code = 0,
            message = "Success"
        }
    }
    
    local resp_data = protobuf.encode("mmo.SendChatResponse", chat_resp)
    gate.send_proto(conn_id, mmo.msg.MSG_SEND_CHAT_RESPONSE, resp_data)
end)

-- 注册心跳处理器
proto_handler.register(mmo.msg.MSG_HEARTBEAT, function(conn_id, data)
    local heartbeat = protobuf.decode("mmo.Heartbeat", data)
    
    -- 回复心跳
    local resp_heartbeat = {
        timestamp = mmo.utils.time_sec()
    }
    
    local resp_data = protobuf.encode("mmo.Heartbeat", resp_heartbeat)
    gate.send_proto(conn_id, mmo.msg.MSG_HEARTBEAT, resp_data)
end)

return proto_handler
