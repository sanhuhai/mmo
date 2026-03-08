#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#endif

namespace mmo {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::string& log_file = "", LogLevel level = LogLevel::Info) {
        level_ = level;
        
#ifdef USE_SPDLOG
        std::vector<spdlog::sink_ptr> sinks;
        
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        sinks.push_back(console_sink);
        
        if (!log_file.empty()) {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file, 1024 * 1024 * 10, 5);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
            sinks.push_back(file_sink);
        }
        
        logger_ = std::make_shared<spdlog::logger>("mmo", sinks.begin(), sinks.end());
        logger_->set_level(ConvertToSpdlogLevel(level));
        logger_->flush_on(spdlog::level::warn);
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);
#else
        if (!log_file.empty()) {
            file_stream_.open(log_file, std::ios::app);
        }
#endif
        initialized_ = true;
    }

    void SetLevel(LogLevel level) {
        level_ = level;
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->set_level(ConvertToSpdlog(level));
        }
#endif
    }

    template<typename... Args>
    void Log(LogLevel level, const char* format, Args&&... args) {
        if (level < level_) return;
        
#ifdef USE_SPDLOG
        if (logger_) {
            switch (level) {
                case LogLevel::Trace: logger_->trace(format, std::forward<Args>(args)...); break;
                case LogLevel::Debug: logger_->debug(format, std::forward<Args>(args)...); break;
                case LogLevel::Info:  logger_->info(format, std::forward<Args>(args)...); break;
                case LogLevel::Warn:  logger_->warn(format, std::forward<Args>(args)...); break;
                case LogLevel::Error: logger_->error(format, std::forward<Args>(args)...); break;
                case LogLevel::Fatal: logger_->critical(format, std::forward<Args>(args)...); break;
            }
        }
#else
        std::lock_guard<std::mutex> lock(mutex_);
        std::string message = FormatString(format, std::forward<Args>(args)...);
        std::string output = FormatLogLine(level, message);
        
        if (file_stream_.is_open()) {
            file_stream_ << output << std::endl;
            file_stream_.flush();
        }
        std::cout << output << std::endl;
#endif
    }

    template<typename... Args>
    void Trace(const char* format, Args&&... args) {
        Log(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Debug(const char* format, Args&&... args) {
        Log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Info(const char* format, Args&&... args) {
        Log(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warn(const char* format, Args&&... args) {
        Log(LogLevel::Warn, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Error(const char* format, Args&&... args) {
        Log(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Fatal(const char* format, Args&&... args) {
        Log(LogLevel::Fatal, format, std::forward<Args>(args)...);
    }

    void Flush() {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->flush();
        }
#else
        if (file_stream_.is_open()) {
            file_stream_.flush();
        }
#endif
    }

private:
    Logger() = default;
    ~Logger() {
#ifdef USE_SPDLOG
        spdlog::shutdown();
#else
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
#endif
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

#ifdef USE_SPDLOG
    spdlog::level::level_enum ConvertToSpdlogLevel(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return spdlog::level::trace;
            case LogLevel::Debug: return spdlog::level::debug;
            case LogLevel::Info:  return spdlog::level::info;
            case LogLevel::Warn:  return spdlog::level::warn;
            case LogLevel::Error: return spdlog::level::err;
            case LogLevel::Fatal: return spdlog::level::critical;
            default: return spdlog::level::info;
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
#else
    std::string FormatString(const char* format) {
        return format;
    }

    template<typename T, typename... Args>
    std::string FormatString(const char* format, T&& value, Args&&... args) {
        std::string result;
        while (*format) {
            if (*format == '{' && *(format + 1) == '}') {
                result += ToString(std::forward<T>(value));
                return result + FormatString(format + 2, std::forward<Args>(args)...);
            }
            result += *format++;
        }
        return result;
    }

    template<typename T>
    std::string ToString(T&& value) {
        std::ostringstream oss;
        oss << std::forward<T>(value);
        return oss.str();
    }

    std::string LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    std::string FormatLogLine(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time_t);
#else
        localtime_r(&time_t, &tm);
#endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S") 
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
            << "[" << LevelToString(level) << "] "
            << message;
        
        return oss.str();
    }

    std::ofstream file_stream_;
    std::mutex mutex_;
#endif

    LogLevel level_ = LogLevel::Info;
    bool initialized_ = false;
};

#define LOG_TRACE(...) mmo::Logger::Instance().Trace(__VA_ARGS__)
#define LOG_DEBUG(...) mmo::Logger::Instance().Debug(__VA_ARGS__)
#define LOG_INFO(...)  mmo::Logger::Instance().Info(__VA_ARGS__)
#define LOG_WARN(...)  mmo::Logger::Instance().Warn(__VA_ARGS__)
#define LOG_ERROR(...) mmo::Logger::Instance().Error(__VA_ARGS__)
#define LOG_FATAL(...) mmo::Logger::Instance().Fatal(__VA_ARGS__)

}
