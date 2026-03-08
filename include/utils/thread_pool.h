#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace mmo {

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop_(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this]() {
                Worker();
            });
        }
    }

    ~ThreadPool() {
        Stop();
    }

    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() {
                (*task)();
            });
        }
        
        cond_.notify_one();
        return result;
    }

    void Stop() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        
        cond_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    size_t GetThreadCount() const { return workers_.size(); }

    size_t GetPendingTaskCount() {
        std::unique_lock<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    void Worker() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [this]() {
                    return stop_ || !tasks_.empty();
                });
                
                if (stop_ && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> stop_;
};

}
