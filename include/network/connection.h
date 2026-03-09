#pragma once

#include "core/logger.h"
#include <memory>
#include <array>
#include <deque>
#include <functional>
#include <asio.hpp>

namespace mmo {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using Ptr = std::shared_ptr<Connection>;
    using MessageHandler = std::function<void(Ptr, const std::vector<uint8_t>&)>;
    using ErrorHandler = std::function<void(Ptr, const std::string&)>;

    enum class State {
        Disconnected,
        Connecting,
        Connected,
        Closing
    };

    Connection(asio::io_context& io_context)
        : socket_(io_context)
        , state_(State::Disconnected)
        , connection_id_(0) {
    }

    ~Connection() {
        Close();
    }

    void SetConnectionId(uint32_t id) { connection_id_ = id; }
    uint32_t GetConnectionId() const { return connection_id_; }

    asio::ip::tcp::socket& GetSocket() { return socket_; }

    void SetMessageHandler(MessageHandler handler) { message_handler_ = handler; }
    void SetErrorHandler(ErrorHandler handler) { error_handler_ = handler; }

    void Start() {
        state_ = State::Connected;
        LOG_DEBUG("Connection {} started", connection_id_);
        AsyncReadHeader();
    }

    void Close() {
        if (state_ == State::Disconnected) {
            return;
        }

        state_ = State::Closing;
        
        asio::error_code ec;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        
        state_ = State::Disconnected;
        LOG_DEBUG("Connection {} closed", connection_id_);
    }

    void Send(const std::vector<uint8_t>& data) {
        if (state_ != State::Connected) {
            return;
        }

        auto self = shared_from_this();
        asio::post(socket_.get_executor(), [this, self, data]() {
            bool write_in_progress = !write_queue_.empty();
            write_queue_.push_back(data);
            
            if (!write_in_progress) {
                AsyncWrite();
            }
        });
    }

    void Send(const uint8_t* data, size_t size) {
        std::vector<uint8_t> buffer(data, data + size);
        Send(buffer);
    }

    State GetState() const { return state_; }
    bool IsConnected() const { return state_ == State::Connected; }

    std::string GetRemoteAddress() const {
        asio::error_code ec;
        auto endpoint = socket_.remote_endpoint(ec);
        if (!ec) {
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        }
        return "";
    }

private:
    void AsyncReadHeader() {
        auto self = shared_from_this();
        asio::async_read(socket_, asio::buffer(header_buffer_),
            [this, self](asio::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    HandleError(ec.message());
                    return;
                }

                uint32_t msg_id = 0;
                uint32_t msg_len = 0;
                std::memcpy(&msg_id, header_buffer_.data(), sizeof(uint32_t));
                std::memcpy(&msg_len, header_buffer_.data() + sizeof(uint32_t), sizeof(uint32_t));

                if (msg_len > 1024 * 1024) {
                    HandleError("Message too large");
                    return;
                }

                LOG_DEBUG("Received header: msg_id={}, msg_len={}", msg_id, msg_len);
                AsyncReadBody(msg_len);
            });
    }

    void AsyncReadBody(uint32_t size) {
        read_buffer_.resize(size);
        
        auto self = shared_from_this();
        asio::async_read(socket_, asio::buffer(read_buffer_),
            [this, self](asio::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    HandleError(ec.message());
                    return;
                }

                std::vector<uint8_t> full_message;
                full_message.reserve(sizeof(uint32_t) * 2 + read_buffer_.size());
                full_message.insert(full_message.end(), header_buffer_.begin(), header_buffer_.end());
                full_message.insert(full_message.end(), read_buffer_.begin(), read_buffer_.end());

                if (message_handler_) {
                    message_handler_(shared_from_this(), full_message);
                }

                AsyncReadHeader();
            });
    }

    void AsyncWrite() {
        if (write_queue_.empty()) {
            return;
        }

        auto self = shared_from_this();
        auto& data = write_queue_.front();
        
        asio::async_write(socket_, asio::buffer(data),
            [this, self](asio::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    HandleError(ec.message());
                    return;
                }

                write_queue_.pop_front();
                if (!write_queue_.empty()) {
                    AsyncWrite();
                }
            });
    }

    void HandleError(const std::string& error) {
        LOG_ERROR("Connection {} error: {}", connection_id_, error);
        
        if (error_handler_) {
            error_handler_(shared_from_this(), error);
        }
        
        Close();
    }

    asio::ip::tcp::socket socket_;
    State state_;
    uint32_t connection_id_;

    std::array<uint8_t, sizeof(uint32_t) * 2> header_buffer_;
    std::vector<uint8_t> read_buffer_;
    std::deque<std::vector<uint8_t>> write_queue_;
    MessageHandler message_handler_;
    ErrorHandler error_handler_;
};

}
