#pragma once

#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>

#include "network/connection.h"
#include "network/connection_manager.h"

namespace mmo {

class TcpServer {
public:
    using Ptr = std::shared_ptr<TcpServer>;
    using AcceptCallback = std::function<void(Connection::Ptr)>;
    using MessageCallback = std::function<void(Connection::Ptr, const std::vector<uint8_t>&)>;

    TcpServer(uint16_t port, size_t thread_count = 4)
        : port_(port)
        , thread_count_(thread_count)
        , acceptor_(io_context_)
        , running_(false) {
        connection_manager_ = std::make_shared<ConnectionManager>();
    }

    ~TcpServer() {
        Stop();
    }

    bool Start() {
        if (running_) {
            return true;
        }

        asio::error_code ec;
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
        
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            LOG_ERROR("Failed to open acceptor: {}", ec.message());
            return false;
        }

        acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
        if (ec) {
            LOG_ERROR("Failed to set reuse_address: {}", ec.message());
            return false;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            LOG_ERROR("Failed to bind to port {}: {}", port_, ec.message());
            return false;
        }

        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            LOG_ERROR("Failed to listen on port {}: {}", port_, ec.message());
            return false;
        }

        running_ = true;
        
        for (size_t i = 0; i < thread_count_; ++i) {
            thread_pool_.emplace_back([this]() {
                io_context_.run();
            });
        }

        AsyncAccept();
        
        LOG_INFO("TCP Server started on port {} with {} worker threads", port_, thread_count_);
        return true;
    }

    void Stop() {
        if (!running_) {
            return;
        }

        running_ = false;
        
        io_context_.stop();
        
        for (auto& thread : thread_pool_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        thread_pool_.clear();
        
        connection_manager_->CloseAll();
        
        asio::error_code ec;
        acceptor_.close(ec);
        
        LOG_INFO("TCP Server stopped");
    }

    void SetAcceptCallback(AcceptCallback callback) {
        on_accept_ = callback;
    }

    void SetMessageCallback(MessageCallback callback) {
        on_message_ = callback;
    }

    ConnectionManager::Ptr GetConnectionManager() {
        return connection_manager_;
    }

    uint16_t GetPort() const { return port_; }
    bool IsRunning() const { return running_; }
    size_t GetConnectionCount() { return connection_manager_->GetConnectionCount(); }

    void Broadcast(const std::vector<uint8_t>& data) {
        connection_manager_->Broadcast(data);
    }

    void Broadcast(const uint8_t* data, size_t size) {
        connection_manager_->Broadcast(data, size);
    }

    void SendTo(uint32_t connection_id, const std::vector<uint8_t>& data) {
        auto conn = connection_manager_->GetConnection(connection_id);
        if (conn && conn->IsConnected()) {
            conn->Send(data);
        }
    }

private:
    void AsyncAccept() {
        if (!running_) {
            return;
        }

        auto conn = std::make_shared<Connection>(io_context_);
        
        acceptor_.async_accept(conn->GetSocket(),
            [this, conn](asio::error_code ec) {
                if (ec) {
                    if (running_) {
                        LOG_ERROR("Accept error: {}", ec.message());
                        AsyncAccept();
                    }
                    return;
                }

                HandleAccept(conn);
                AsyncAccept();
            });
    }

    void HandleAccept(Connection::Ptr conn) {
        connection_manager_->AddConnection(conn);
        
        conn->SetMessageHandler([this](Connection::Ptr c, const std::vector<uint8_t>& data) {
            if (on_message_) {
                on_message_(c, data);
            }
        });

        conn->SetErrorHandler([this](Connection::Ptr c, const std::string& error) {
            connection_manager_->RemoveConnection(c);
        });

        conn->Start();
        
        LOG_INFO("New connection from {}", conn->GetRemoteAddress());
        
        if (on_accept_) {
            on_accept_(conn);
        }
    }

    uint16_t port_;
    size_t thread_count_;
    asio::io_context io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::vector<std::thread> thread_pool_;
    std::atomic<bool> running_;

    ConnectionManager::Ptr connection_manager_;

    AcceptCallback on_accept_;
    MessageCallback on_message_;
};

}
