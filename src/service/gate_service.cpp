#include "service/gate_service.h"
#include "script/lua_register.h"

namespace mmo {

REGISTER_SERVICE("gate", GateService);

GateService::GateService(uint32_t id, const std::string& name)
    : ServiceBase(id, name)
    , port_(8888) {
}

GateService::~GateService() {
    Shutdown();
}

bool GateService::Initialize() {
    LOG_INFO("Initializing GateService '{}' on port {}", name_, port_);

    if (!lua_engine_.Initialize()) {
        LOG_ERROR("Failed to initialize Lua engine for GateService");
        return false;
    }

    RegisterLuaFunctions();

    std::string script_path = "lua/service/" + name_;
    lua_engine_.SetScriptPath(script_path);
    
    std::string init_script = script_path + "/init.lua";
    if (!lua_engine_.DoFile(init_script)) {
        LOG_WARN("GateService init script not found: {}", init_script);
    }

    tcp_server_ = std::make_unique<TcpServer>(port_, 4);
    
    tcp_server_->SetAcceptCallback([this](Connection::Ptr conn) {
        OnNewConnection(conn);
    });

    tcp_server_->SetMessageCallback([this](Connection::Ptr conn, const std::vector<uint8_t>& data) {
        OnMessage(conn, data);
    });

    if (!tcp_server_->Start()) {
        LOG_ERROR("Failed to start TCP server on port {}", port_);
        return false;
    }

    running_ = true;
    LOG_INFO("GateService '{}' initialized successfully", name_);
    return true;
}

void GateService::Update(int64_t delta_ms) {
    ServiceMessage msg;
    while (PopMessage(msg)) {
        if (message_handler_) {
            message_handler_(msg);
        }

        lua_engine_.Call("on_service_message", msg.source, msg.type, 
            std::string(msg.data.begin(), msg.data.end()));
    }

    lua_engine_.Call("on_update", delta_ms);
}

void GateService::Shutdown() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (tcp_server_) {
        tcp_server_->Stop();
        tcp_server_.reset();
    }

    client_connections_.clear();

    lua_engine_.Shutdown();

    LOG_INFO("GateService '{}' shutdown", name_);
}

void GateService::OnNewConnection(Connection::Ptr conn) {
    uint32_t conn_id = conn->GetConnectionId();
    client_connections_[conn_id] = conn;

    LOG_INFO("New client connection: {} (total: {})", conn->GetRemoteAddress(), client_connections_.size());

    lua_engine_.Call("on_client_connect", conn_id, conn->GetRemoteAddress());
}

void GateService::OnMessage(Connection::Ptr conn, const std::vector<uint8_t>& data) {
    uint32_t conn_id = conn->GetConnectionId();

    std::string msg_str(data.begin(), data.end());
    lua_engine_.Call("on_client_message", conn_id, msg_str);
}

void GateService::OnDisconnect(Connection::Ptr conn) {
    uint32_t conn_id = conn->GetConnectionId();
    client_connections_.erase(conn_id);

    LOG_INFO("Client disconnected: {} (total: {})", conn_id, client_connections_.size());

    lua_engine_.Call("on_client_disconnect", conn_id);
}

void GateService::SendToClient(uint32_t connection_id, const std::vector<uint8_t>& data) {
    auto it = client_connections_.find(connection_id);
    if (it != client_connections_.end() && it->second->IsConnected()) {
        it->second->Send(data);
    }
}

void GateService::BroadcastToClients(const std::vector<uint8_t>& data) {
    for (auto& pair : client_connections_) {
        if (pair.second->IsConnected()) {
            pair.second->Send(data);
        }
    }
}

void GateService::KickClient(uint32_t connection_id) {
    auto it = client_connections_.find(connection_id);
    if (it != client_connections_.end()) {
        it->second->Close();
        client_connections_.erase(it);
    }
}

size_t GateService::GetClientCount() const {
    return client_connections_.size();
}

void GateService::RegisterLuaFunctions() {
    auto* self = this;

    luabridge::getGlobalNamespace(lua_engine_.GetState())
        .beginNamespace("gate")
        .endNamespace();
}

}
