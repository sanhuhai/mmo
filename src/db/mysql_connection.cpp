#include "db/mysql_connection.h"

#include <cstring>

#ifdef USE_MYSQL

namespace mmo {

MySQLConnection::MySQLConnection() {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        LOG_ERROR("Failed to initialize MySQL connection");
    }
}

MySQLConnection::~MySQLConnection() {
    Disconnect();
}

bool MySQLConnection::Connect(const std::string& host, uint16_t port,
                               const std::string& user, const std::string& password,
                               const std::string& database) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        LOG_ERROR("MySQL not initialized");
        return false;
    }

    host_ = host;
    port_ = port;
    user_ = user;
    password_ = password;
    database_ = database;

    MYSQL* result = mysql_real_connect(mysql_, host.c_str(), user.c_str(),
                                       password.c_str(), database.c_str(),
                                       port, nullptr, 0);
    if (!result) {
        LOG_ERROR("Failed to connect to MySQL: {}", mysql_error(mysql_));
        return false;
    }

    mysql_set_character_set(mysql_, "utf8mb4");
    
    LOG_INFO("Connected to MySQL server {}:{}/{} as {}", host, port, database, user);
    return true;
}

void MySQLConnection::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        LOG_INFO("Disconnected from MySQL");
    }
}

bool MySQLConnection::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mysql_ && (mysql_ping(mysql_) == 0);
}

bool MySQLConnection::Execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        LOG_ERROR("MySQL not connected");
        return false;
    }

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        LOG_ERROR("MySQL query failed: {}", mysql_error(mysql_));
        return false;
    }

    return true;
}

bool MySQLConnection::Execute(const std::string& sql, const std::vector<std::string>& params) {
    MYSQL_STMT* stmt = nullptr;
    if (!PrepareStatement(sql, params, &stmt)) {
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        LOG_ERROR("MySQL statement execute failed: {}", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

MySQLResult MySQLConnection::Query(const std::string& sql) {
    MySQLResult result;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        result.error_message = "MySQL not connected";
        return result;
    }

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        result.error_message = mysql_error(mysql_);
        LOG_ERROR("MySQL query failed: {}", result.error_message);
        return result;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        result.affected_rows = mysql_affected_rows(mysql_);
        result.success = true;
        return result;
    }

    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    unsigned int num_fields = mysql_num_fields(res);
    
    for (unsigned int i = 0; i < num_fields; ++i) {
        result.columns.push_back(fields[i].name);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::vector<std::string> row_data;
        for (unsigned int i = 0; i < num_fields; ++i) {
            row_data.push_back(row[i] ? row[i] : "");
        }
        result.rows.push_back(row_data);
    }

    mysql_free_result(res);
    result.success = true;
    return result;
}

MySQLResult MySQLConnection::Query(const std::string& sql, const std::vector<std::string>& params) {
    MySQLResult result;
    
    MYSQL_STMT* stmt = nullptr;
    if (!PrepareStatement(sql, params, &stmt)) {
        result.error_message = "Failed to prepare statement";
        return result;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        result.error_message = mysql_stmt_error(stmt);
        LOG_ERROR("MySQL statement execute failed: {}", result.error_message);
        mysql_stmt_close(stmt);
        return result;
    }

    result = ExecuteQuery(stmt);
    mysql_stmt_close(stmt);
    return result;
}

uint64_t MySQLConnection::GetAffectedRows() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mysql_ ? mysql_affected_rows(mysql_) : 0;
}

uint64_t MySQLConnection::GetInsertId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mysql_ ? mysql_insert_id(mysql_) : 0;
}

std::string MySQLConnection::GetLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mysql_ ? mysql_error(mysql_) : "Not connected";
}

bool MySQLConnection::BeginTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        return false;
    }

    if (mysql_query(mysql_, "START TRANSACTION") != 0) {
        LOG_ERROR("Failed to begin transaction: {}", mysql_error(mysql_));
        return false;
    }

    in_transaction_ = true;
    return true;
}

bool MySQLConnection::Commit() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        return false;
    }

    if (mysql_query(mysql_, "COMMIT") != 0) {
        LOG_ERROR("Failed to commit transaction: {}", mysql_error(mysql_));
        return false;
    }

    in_transaction_ = false;
    return true;
}

bool MySQLConnection::Rollback() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_) {
        return false;
    }

    if (mysql_query(mysql_, "ROLLBACK") != 0) {
        LOG_ERROR("Failed to rollback transaction: {}", mysql_error(mysql_));
        return false;
    }

    in_transaction_ = false;
    return true;
}

bool MySQLConnection::Ping() {
    std::lock_guard<std::mutex> lock(mutex_);
    return mysql_ && (mysql_ping(mysql_) == 0);
}

bool MySQLConnection::Reconnect() {
    Disconnect();
    
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        return false;
    }

    return Connect(host_, port_, user_, password_, database_);
}

bool MySQLConnection::PrepareStatement(const std::string& sql, const std::vector<std::string>& params, MYSQL_STMT** stmt) {
    *stmt = mysql_stmt_init(mysql_);
    if (!*stmt) {
        LOG_ERROR("Failed to initialize statement");
        return false;
    }

    if (mysql_stmt_prepare(*stmt, sql.c_str(), sql.length()) != 0) {
        LOG_ERROR("Failed to prepare statement: {}", mysql_stmt_error(*stmt));
        mysql_stmt_close(*stmt);
        *stmt = nullptr;
        return false;
    }

    if (params.empty()) {
        return true;
    }

    std::vector<MYSQL_BIND> binds(params.size());
    std::vector<unsigned long> lengths(params.size());

    for (size_t i = 0; i < params.size(); ++i) {
        memset(&binds[i], 0, sizeof(MYSQL_BIND));
        lengths[i] = params[i].length();
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = (void*)params[i].c_str();
        binds[i].buffer_length = lengths[i];
        binds[i].length = &lengths[i];
    }

    if (mysql_stmt_bind_param(*stmt, binds.data()) != 0) {
        LOG_ERROR("Failed to bind parameters: {}", mysql_stmt_error(*stmt));
        mysql_stmt_close(*stmt);
        *stmt = nullptr;
        return false;
    }

    return true;
}

MySQLResult MySQLConnection::ExecuteQuery(MYSQL_STMT* stmt) {
    MySQLResult result;

    if (mysql_stmt_execute(stmt) != 0) {
        result.error_message = mysql_stmt_error(stmt);
        LOG_ERROR("MySQL statement execute failed: {}", result.error_message);
        return result;
    }

    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) {
        result.affected_rows = mysql_stmt_affected_rows(stmt);
        result.insert_id = mysql_stmt_insert_id(stmt);
        result.success = true;
        return result;
    }

    unsigned int num_fields = mysql_num_fields(meta);
    MYSQL_FIELD* fields = mysql_fetch_fields(meta);

    for (unsigned int i = 0; i < num_fields; ++i) {
        result.columns.push_back(fields[i].name);
    }

    std::vector<MYSQL_BIND> binds(num_fields);
    std::vector<std::vector<char>> buffers(num_fields);
    std::vector<unsigned long> lengths(num_fields);
    std::vector<char> is_null(num_fields);

    for (unsigned int i = 0; i < num_fields; ++i) {
        buffers[i].resize(1024);
        memset(&binds[i], 0, sizeof(MYSQL_BIND));
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = buffers[i].data();
        binds[i].buffer_length = buffers[i].size();
        binds[i].length = &lengths[i];
        binds[i].is_null = reinterpret_cast<bool*>(&is_null[i]);
    }

    if (mysql_stmt_bind_result(stmt, binds.data()) != 0) {
        result.error_message = mysql_stmt_error(stmt);
        LOG_ERROR("Failed to bind result: {}", result.error_message);
        mysql_free_result(meta);
        return result;
    }

    while (mysql_stmt_fetch(stmt) == 0) {
        std::vector<std::string> row;
        for (unsigned int i = 0; i < num_fields; ++i) {
            if (is_null[i]) {
                row.push_back("");
            } else {
                row.push_back(std::string(buffers[i].data(), lengths[i]));
            }
        }
        result.rows.push_back(row);
    }

    mysql_free_result(meta);
    result.success = true;
    return result;
}

}

#endif
