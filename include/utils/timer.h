#pragma once

#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <set>

namespace mmo {

class Timer {
public:
    using TimerId = uint64_t;
    using TimerCallback = std::function<void()>;

    struct TimerEntry {
        TimerId id;
        std::chrono::steady_clock::time_point expire_time;
        TimerCallback callback;
        bool repeat;
        std::chrono::milliseconds interval;

        bool operator>(const TimerEntry& other) const {
            return expire_time > other.expire_time;
        }
    };

    Timer() : next_id_(1), running_(true) {
        worker_thread_ = std::thread(&Timer::Worker, this);
    }

    ~Timer() {
        Stop();
    }

    TimerId AddTimer(std::chrono::milliseconds delay, TimerCallback callback, bool repeat = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        TimerEntry entry;
        entry.id = next_id_++;
        entry.expire_time = std::chrono::steady_clock::now() + delay;
        entry.callback = callback;
        entry.repeat = repeat;
        entry.interval = delay;
        
        timers_.push(entry);
        cond_.notify_one();
        
        return entry.id;
    }

    void CancelTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(mutex_);
        cancelled_.insert(id);
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cond_.notify_all();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    size_t GetTimerCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        return timers_.size();
    }

private:
    void Worker() {
        while (running_) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (timers_.empty()) {
                cond_.wait(lock, [this]() {
                    return !timers_.empty() || !running_;
                });
                
                if (!running_) break;
            }
            
            auto now = std::chrono::steady_clock::now();
            
            while (!timers_.empty() && timers_.top().expire_time <= now) {
                TimerEntry entry = timers_.top();
                timers_.pop();
                
                if (cancelled_.find(entry.id) != cancelled_.end()) {
                    cancelled_.erase(entry.id);
                    continue;
                }
                
                lock.unlock();
                if (entry.callback) {
                    entry.callback();
                }
                lock.lock();
                
                if (entry.repeat && running_) {
                    entry.expire_time = std::chrono::steady_clock::now() + entry.interval;
                    timers_.push(entry);
                }
            }
            
            if (!timers_.empty()) {
                auto sleep_time = timers_.top().expire_time - std::chrono::steady_clock::now();
                if (sleep_time > std::chrono::milliseconds(0)) {
                    cond_.wait_for(lock, sleep_time);
                }
            }
        }
    }

    std::priority_queue<TimerEntry, std::vector<TimerEntry>, std::greater<TimerEntry>> timers_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread worker_thread_;
    TimerId next_id_;
    std::atomic<bool> running_;
    std::set<TimerId> cancelled_;
};

}
