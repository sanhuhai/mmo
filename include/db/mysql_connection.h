#pragma once

#ifdef USE_MYSQL

#include "core/logger.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <mysql.h>

namespace mmo {

struct MySQLResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    uint64_t affected_rows = 0;
    uint64_t insert_id = 0;
    bool success = false;
    std::string error_message;
};

class MySQLConnection {
public:
    using Ptr = std::shared_ptr<MySQLConnection>;

    MySQLConnection();
    ~MySQLConnection();

    bool Connect(const std::string& host, uint16_t port,
                 const std::string& user, const std::string& password,
                 const std::string& database);
    void Disconnect();
    bool IsConnected() const;

    bool Execute(const std::string& sql);
    bool Execute(const std::string& sql, const std::vector<std::string>& params);
    
    MySQLResult Query(const std::string& sql);
    MySQLResult Query(const std::string& sql, const std::vector<std::string>& params);

    uint64_t GetAffectedRows() const;
    uint64_t GetInsertId() const;
    std::string GetLastError() const;

    bool BeginTransaction();
    bool Commit();
    bool Rollback();

    bool Ping();
    bool Reconnect();

    const std::string& GetHost() const { return host_; }
    uint16_t GetPort() const { return port_; }
    const std::string& GetDatabase() const { return database_; }

private:
    bool PrepareStatement(const std::string& sql, const std::vector<std::string>& params, MYSQL_STMT** stmt);
    MySQLResult ExecuteQuery(MYSQL_STMT* stmt);

    MYSQL* mysql_ = nullptr;
    std::string host_;
    uint16_t port_ = 3306;
    std::string user_;
    std::string password_;
    std::string database_;
    
    mutable std::mutex mutex_;
    bool in_transaction_ = false;
};

}

#endif
