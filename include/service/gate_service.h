#pragma once

#include <memory>
#include <unordered_map>

#include "service/service_base.h"
#include "network/tcp_server.h"

namespace mmo {

class GateService : public ServiceBase {
public:
    GateService(uint32_t id, const std::string& name);
    ~GateService() override;

    bool Initialize() override;
    void Update(int64_t delta_ms) override;
    void Shutdown() override;

    void SetPort(uint16_t port) { port_ = port; }
    uint16_t GetPort() const { return port_; }

    void OnNewConnection(Connection::Ptr conn);
    void OnMessage(Connection::Ptr conn, const std::vector<uint8_t>& data);
    void OnDisconnect(Connection::Ptr conn);

    void SendToClient(uint32_t connection_id, const std::vector<uint8_t>& data);
    void BroadcastToClients(const std::vector<uint8_t>& data);
    void KickClient(uint32_t connection_id);

    size_t GetClientCount() const;

private:
    void RegisterLuaFunctions();

    uint16_t port_;
    std::unique_ptr<TcpServer> tcp_server_;
    std::unordered_map<uint32_t, Connection::Ptr> client_connections_;
};

}
