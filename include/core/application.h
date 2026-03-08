#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

#include "core/logger.h"
#include "core/config.h"

namespace mmo {

class Application {
public:
    using SignalHandler = std::function<void()>;

    static Application& Instance() {
        static Application instance;
        return instance;
    }

    bool Initialize(int argc, char* argv[]) {
        LOG_INFO("Initializing MMO Server...");

        if (!ParseCommandLine(argc, argv)) {
            return false;
        }

        std::string config_file = config_file_.empty() ? "config/server.ini" : config_file_;
        if (!Config::Instance().Load(config_file)) {
            LOG_WARN("Config file {} not found, using defaults", config_file);
        }

        std::string log_file = Config::Instance().Get("log.file", ConfigValue("logs/server.log")).AsString();
        LogLevel log_level = static_cast<LogLevel>(
            Config::Instance().Get("log.level", ConfigValue("2")).AsInt());
        Logger::Instance().Initialize(log_file, log_level);

        server_name_ = Config::Instance().Get("server.name", ConfigValue("MMO-Server")).AsString();
        server_id_ = Config::Instance().Get("server.id", ConfigValue("1")).AsInt();

        LOG_INFO("Server '{}' (ID: {}) initialized", server_name_, server_id_);
        return true;
    }

    int Run() {
        LOG_INFO("Starting MMO Server main loop...");
        
        running_ = true;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (running_) {
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
            
            for (auto& callback : update_callbacks_) {
                callback(delta.count());
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        LOG_INFO("Server stopped");
        return 0;
    }

    void Stop() {
        LOG_INFO("Stopping server...");
        running_ = false;
    }

    void RegisterUpdateCallback(std::function<void(int64_t)> callback) {
        update_callbacks_.push_back(callback);
    }

    void RegisterSignalHandler(SignalHandler handler) {
        signal_handlers_.push_back(handler);
    }

    const std::string& GetServerName() const { return server_name_; }
    int GetServerId() const { return server_id_; }
    bool IsRunning() const { return running_; }

private:
    Application() = default;
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool ParseCommandLine(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-c" || arg == "--config") {
                if (i + 1 < argc) {
                    config_file_ = argv[++i];
                }
            } else if (arg == "-h" || arg == "--help") {
                PrintUsage();
                return false;
            }
        }
        return true;
    }

    void PrintUsage() {
        std::cout << "Usage: mmo_server [options]\n"
                  << "Options:\n"
                  << "  -c, --config <file>  Specify config file (default: config/server.ini)\n"
                  << "  -h, --help           Show this help message\n";
    }

    std::string server_name_;
    int server_id_ = 1;
    std::string config_file_;
    std::atomic<bool> running_{false};
    std::vector<std::function<void(int64_t)>> update_callbacks_;
    std::vector<SignalHandler> signal_handlers_;
};

}
