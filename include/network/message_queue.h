#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include "core/logger.h"

namespace mmo {

template<typename T>
class MessageQueue {
public:
    using MessageHandler = std::function<void(const T&)>;

    MessageQueue(size_t max_size = 10000)
        : max_size_(max_size)
        , running_(false)
        , stopped_(false) {
    }

    ~MessageQueue() {
        Stop();
    }

    void Start() {
        if (running_) {
            return;
        }

        running_ = true;
        stopped_ = false;
        consumer_thread_ = std::thread(&MessageQueue::ConsumerLoop, this);
        LOG_INFO("Message queue started");
    }

    void Stop() {
        if (!running_) {
            return;
        }

        running_ = false;
        stopped_ = true;
        cv_.notify_all();

        if (consumer_thread_.joinable()) {
            consumer_thread_.join();
        }

        LOG_INFO("Message queue stopped");
    }

    bool Push(const T& message) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (queue_.size() >= max_size_) {
            LOG_WARN("Message queue is full, dropping message");
            return false;
        }

        queue_.push(message);
        lock.unlock();
        cv_.notify_one();
        return true;
    }

    bool Push(T&& message) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (queue_.size() >= max_size_) {
            LOG_WARN("Message queue is full, dropping message");
            return false;
        }

        queue_.push(std::move(message));
        lock.unlock();
        cv_.notify_one();
        return true;
    }

    bool TryPop(T& message) {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (queue_.empty()) {
            return false;
        }

        message = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size();
    }

    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.empty();
    }

    bool IsFull() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size() >= max_size_;
    }

    void SetHandler(MessageHandler handler) {
        handler_ = handler;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    void ConsumerLoop() {
        LOG_INFO("Message queue consumer thread started");

        while (running_ && !stopped_) {
            T message;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this] {
                    return !queue_.empty() || stopped_;
                });

                if (stopped_) {
                    break;
                }

                if (queue_.empty()) {
                    continue;
                }

                message = std::move(queue_.front());
                queue_.pop();
            }

            if (handler_) {
                try {
                    handler_(message);
                } catch (const std::exception& e) {
                    LOG_ERROR("Message handler error: {}", e.what());
                }
            }
        }

        LOG_INFO("Message queue consumer thread stopped");
    }

private:
    std::queue<T> queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread consumer_thread_;
    MessageHandler handler_;
    size_t max_size_;
    std::atomic<bool> running_;
    std::atomic<bool> stopped_;
};

}
