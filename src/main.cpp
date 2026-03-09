#include <iostream>
#include <signal.h>

#include "core/application.h"
#include "core/logger.h"
#include "core/config.h"
#include "service/service_manager.h"
#include "script/lua_engine.h"
#include "script/lua_register.h"
#include "player/player_manager.h"
#include "player/weapon.h"

#ifdef USE_MYSQL
#include "db/mysql_pool.h"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {
    std::atomic<bool> g_running(true);

    void SignalHandler(int signal) {
        LOG_INFO("Received signal {}, shutting down...", signal);
        g_running = false;
    }

#ifdef _WIN32
    BOOL WINAPI ConsoleHandler(DWORD fdwCtrlType) {
        switch (fdwCtrlType) {
            case CTRL_C_EVENT:
            case CTRL_CLOSE_EVENT:
            case CTRL_BREAK_EVENT:
                LOG_INFO("Received console event, shutting down...");
                g_running = false;
                return TRUE;
            default:
                return FALSE;
        }
    }
#endif
}

int main(int argc, char* argv[]) {
    try {
#ifdef _WIN32
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);
#endif

        if (!mmo::Application::Instance().Initialize(argc, argv)) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }

        mmo::Logger::Instance().Initialize("../../logs/server.log", mmo::LogLevel::Debug);



        mmo::LuaEngine lua_engine;
        if (!lua_engine.Initialize()) {
            LOG_ERROR("Failed to initialize Lua engine");
            return 1;
        }

        mmo::LuaRegister::RegisterAll(lua_engine);

#ifdef USE_MYSQL
        mmo::MySQLConfig mysql_config;
        mmo::Config::Instance().Load("../../config/server.ini");
        mysql_config.host = mmo::Config::Instance().Get("mysql.host", mmo::ConfigValue("localhost")).AsString();
        mysql_config.port = mmo::Config::Instance().Get("mysql.port", mmo::ConfigValue("3306")).AsInt();
        mysql_config.user = mmo::Config::Instance().Get("mysql.user", mmo::ConfigValue("root")).AsString();
        mysql_config.password = mmo::Config::Instance().Get("mysql.password", mmo::ConfigValue("1234567890yy")).AsString();
        mysql_config.database = mmo::Config::Instance().Get("mysql.database", mmo::ConfigValue("mmo_server")).AsString();
        mysql_config.pool_size = mmo::Config::Instance().Get("mysql.pool_size", mmo::ConfigValue("10")).AsInt();

        LOG_INFO("Initializing MySQL connection pool...");
        if (!mmo::PlayerManager::Instance().Initialize(mysql_config)) {
            LOG_ERROR("Failed to initialize PlayerManager");
            return 1;
        }

        LOG_INFO("Initializing database tables...");
        if (!mmo::PlayerManager::Instance().InitializeDatabaseTables()) {
            LOG_ERROR("Failed to initialize database tables");
            return 1;
        }
#endif

        lua_engine.SetScriptPath("../lua");

        if (!lua_engine.DoFile("../lua/main.lua")) {
            LOG_WARN("Main Lua script not found or has errors");
        }

        if (!mmo::ServiceManager::Instance().Initialize()) {
            LOG_ERROR("Failed to initialize ServiceManager");
            return 1;
        }

        auto gate_service = mmo::ServiceManager::Instance().CreateService("gate", "gate1");
        if (gate_service) {
            auto gate = std::dynamic_pointer_cast<mmo::GateService>(gate_service);
            if (gate) {
                uint16_t port = mmo::Config::Instance().Get("gate.port", mmo::ConfigValue("8818")).AsInt();
                gate->SetPort(port);
            }
        }

        auto game_service = mmo::ServiceManager::Instance().CreateService("game", "game1");

        LOG_INFO("MMO Server started successfully");
        LOG_INFO("Services: {}", mmo::ServiceManager::Instance().GetServiceCount());

        lua_engine.Call("on_server_start");

        auto start_time = std::chrono::steady_clock::now();
        auto last_update = start_time;

        while (g_running) {
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
            
            if (delta.count() >= 16) {
                last_update = now;
                
                mmo::ServiceManager::Instance().UpdateAll(delta.count());
                
                lua_engine.Call("on_update", delta.count());
            }

#ifdef _WIN32
            Sleep(1);
#else
            usleep(1000);
#endif
        }

        lua_engine.Call("on_server_stop");

#ifdef USE_MYSQL
        mmo::PlayerManager::Instance().Shutdown();
#endif

    

        mmo::ServiceManager::Instance().Shutdown();
        lua_engine.Shutdown();

        LOG_INFO("MMO Server stopped");
        mmo::Logger::Instance().Flush();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
