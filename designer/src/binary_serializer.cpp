#include "excel_converter/binary_serializer.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace excel_converter {

void BinaryData::WriteInt32(int32_t value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(int32_t));
}

void BinaryData::WriteUInt32(uint32_t value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(uint32_t));
}

void BinaryData::WriteInt64(int64_t value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(int64_t));
}

void BinaryData::WriteUInt64(uint64_t value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(uint64_t));
}

void BinaryData::WriteFloat(float value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(float));
}

void BinaryData::WriteDouble(double value) {
    data.insert(data.end(), 
                reinterpret_cast<uint8_t*>(&value), 
                reinterpret_cast<uint8_t*>(&value) + sizeof(double));
}

void BinaryData::WriteString(const std::string& value) {
    uint32_t len = static_cast<uint32_t>(value.size());
    WriteUInt32(len);
    data.insert(data.end(), value.begin(), value.end());
}

void BinaryData::WriteBool(bool value) {
    data.push_back(value ? 1 : 0);
}

void BinaryData::WriteBytes(const std::vector<uint8_t>& value) {
    uint32_t len = static_cast<uint32_t>(value.size());
    WriteUInt32(len);
    data.insert(data.end(), value.begin(), value.end());
}

void BinaryData::WriteLength(uint32_t length) {
    WriteUInt32(length);
}

int32_t BinaryData::ReadInt32(size_t& offset) const {
    if (offset + sizeof(int32_t) > data.size()) return 0;
    int32_t value;
    std::memcpy(&value, data.data() + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    return value;
}

uint32_t BinaryData::ReadUInt32(size_t& offset) const {
    if (offset + sizeof(uint32_t) > data.size()) return 0;
    uint32_t value;
    std::memcpy(&value, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return value;
}

int64_t BinaryData::ReadInt64(size_t& offset) const {
    if (offset + sizeof(int64_t) > data.size()) return 0;
    int64_t value;
    std::memcpy(&value, data.data() + offset, sizeof(int64_t));
    offset += sizeof(int64_t);
    return value;
}

uint64_t BinaryData::ReadUInt64(size_t& offset) const {
    if (offset + sizeof(uint64_t) > data.size()) return 0;
    uint64_t value;
    std::memcpy(&value, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return value;
}

float BinaryData::ReadFloat(size_t& offset) const {
    if (offset + sizeof(float) > data.size()) return 0.0f;
    float value;
    std::memcpy(&value, data.data() + offset, sizeof(float));
    offset += sizeof(float);
    return value;
}

double BinaryData::ReadDouble(size_t& offset) const {
    if (offset + sizeof(double) > data.size()) return 0.0;
    double value;
    std::memcpy(&value, data.data() + offset, sizeof(double));
    offset += sizeof(double);
    return value;
}

std::string BinaryData::ReadString(size_t& offset) const {
    uint32_t len = ReadUInt32(offset);
    if (offset + len > data.size()) return "";
    std::string value(reinterpret_cast<const char*>(data.data() + offset), len);
    offset += len;
    return value;
}

bool BinaryData::ReadBool(size_t& offset) const {
    if (offset >= data.size()) return false;
    return data[offset++] != 0;
}

std::vector<uint8_t> BinaryData::ReadBytes(size_t& offset) const {
    uint32_t len = ReadUInt32(offset);
    if (offset + len > data.size()) return {};
    std::vector<uint8_t> value(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return value;
}

bool BinarySerializer::SerializeRow(const TableData& table,
                                   const std::vector<std::string>& row,
                                   BinaryData& out_data) {
    out_data.data.clear();
    
    for (size_t i = 0; i < table.fields.size(); ++i) {
        const auto& field = table.fields[i];
        std::string value = (i < row.size()) ? row[i] : "";
        
        if (!SerializeField(field, value, out_data)) {
            return false;
        }
    }
    
    return true;
}

bool BinarySerializer::SerializeTable(const TableData& table,
                                     std::vector<BinaryData>& out_rows) {
    out_rows.clear();
    out_rows.reserve(table.rows.size());
    
    for (const auto& row : table.rows) {
        BinaryData data;
        if (!SerializeRow(table, row, data)) {
            return false;
        }
        out_rows.push_back(std::move(data));
    }
    
    return true;
}

bool BinarySerializer::SerializeField(const FieldInfo& field,
                                     const std::string& value,
                                     BinaryData& out_data) {
    if (field.is_array) {
        auto values = ParseArrayValue(value);
        out_data.WriteUInt32(static_cast<uint32_t>(values.size()));
        
        for (const auto& v : values) {
            if (!ParseValue(v, field.type, out_data)) {
                return false;
            }
        }
    } else {
        if (!ParseValue(value, field.type, out_data)) {
            return false;
        }
    }
    
    return true;
}

bool BinarySerializer::ParseValue(const std::string& value, FieldType type, BinaryData& out_data) {
    try {
        switch (type) {
            case FieldType::INT32:
                out_data.WriteInt32(value.empty() ? 0 : std::stoi(value));
                break;
            case FieldType::INT64:
                out_data.WriteInt64(value.empty() ? 0 : std::stoll(value));
                break;
            case FieldType::UINT32:
                out_data.WriteUInt32(value.empty() ? 0 : static_cast<uint32_t>(std::stoul(value)));
                break;
            case FieldType::UINT64:
                out_data.WriteUInt64(value.empty() ? 0 : std::stoull(value));
                break;
            case FieldType::FLOAT:
                out_data.WriteFloat(value.empty() ? 0.0f : std::stof(value));
                break;
            case FieldType::DOUBLE:
                out_data.WriteDouble(value.empty() ? 0.0 : std::stod(value));
                break;
            case FieldType::STRING:
                out_data.WriteString(value);
                break;
            case FieldType::BOOL: {
                std::string lower = value;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                bool b = (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
                out_data.WriteBool(b);
                break;
            }
            case FieldType::BYTES:
                out_data.WriteBytes(std::vector<uint8_t>(value.begin(), value.end()));
                break;
            default:
                out_data.WriteString(value);
                break;
        }
    } catch (const std::exception& e) {
        return false;
    }
    
    return true;
}

std::vector<std::string> BinarySerializer::ParseArrayValue(const std::string& value) {
    std::vector<std::string> result;
    
    if (value.empty()) {
        return result;
    }
    
    char delimiter = ',';
    if (value.find('|') != std::string::npos) {
        delimiter = '|';
    } else if (value.find(';') != std::string::npos) {
        delimiter = ';';
    }
    
    std::stringstream ss(value);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        size_t start = item.find_first_not_of(" \t\r\n");
        size_t end = item.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    
    return result;
}

SQLiteStorage::SQLiteStorage() : db_(nullptr) {
}

SQLiteStorage::~SQLiteStorage() {
    Close();
}

bool SQLiteStorage::Open(const std::string& db_path) {
    Close();
    
    int result = sqlite3_open(db_path.c_str(), &db_);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to open SQLite database: " + std::string(sqlite3_errmsg(db_));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    char* err_msg = nullptr;
    result = sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        if (err_msg) sqlite3_free(err_msg);
    }
    
    result = sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        if (err_msg) sqlite3_free(err_msg);
    }
    
    return true;
}

void SQLiteStorage::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SQLiteStorage::CreateTable(const TableData& table) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    if (TableExists(table.table_name)) {
        if (!DropTable(table.table_name)) {
            return false;
        }
    }
    
    std::string sql = GetCreateTableSQL(table);
    
    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (result != SQLITE_OK) {
        last_error_ = "Failed to create table: " + std::string(err_msg ? err_msg : "unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteStorage::InsertRow(const TableData& table,
                             const std::vector<std::string>& row,
                             const BinaryData& binary_data) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    std::string sql = GetInsertSQL(table);
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        last_error_ = "Failed to prepare statement: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    int param_index = 1;
    for (size_t i = 0; i < table.fields.size() && i < row.size(); ++i) {
        sqlite3_bind_text(stmt, param_index++, row[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    
    sqlite3_bind_blob(stmt, param_index, binary_data.data.data(), 
                      static_cast<int>(binary_data.data.size()), SQLITE_TRANSIENT);
    
    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (result != SQLITE_DONE) {
        last_error_ = "Failed to insert row: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    return true;
}

bool SQLiteStorage::InsertTable(const TableData& table,
                               const std::vector<BinaryData>& binary_rows) {
    if (!db_) {
        last_error_ = "Database not opened";
        return false;
    }
    
    if (!BeginTransaction()) {
        return false;
    }
    
    for (size_t i = 0; i < table.rows.size(); ++i) {
        if (!InsertRow(table, table.rows[i], binary_rows[i])) {
            RollbackTransaction();
            return false;
        }
    }
    
    return CommitTransaction();
}

bool SQLiteStorage::TableExists(const std::string& table_name) {
    if (!db_) return false;
    
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);
    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return result == SQLITE_ROW;
}

bool SQLiteStorage::DropTable(const std::string& table_name) {
    if (!db_) return false;
    
    std::string sql = "DROP TABLE IF EXISTS \"" + table_name + "\";";
    char* err_msg = nullptr;
    
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to drop table: " + std::string(err_msg ? err_msg : "unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteStorage::ClearTable(const std::string& table_name) {
    if (!db_) return false;
    
    std::string sql = "DELETE FROM \"" + table_name + "\";";
    char* err_msg = nullptr;
    
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to clear table: " + std::string(err_msg ? err_msg : "unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteStorage::BeginTransaction() {
    if (!db_) return false;
    
    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to begin transaction: " + std::string(err_msg ? err_msg : "unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteStorage::CommitTransaction() {
    if (!db_) return false;
    
    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err_msg);
    if (result != SQLITE_OK) {
        last_error_ = "Failed to commit transaction: " + std::string(err_msg ? err_msg : "unknown error");
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SQLiteStorage::RollbackTransaction() {
    if (!db_) return false;
    
    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err_msg);
    if (err_msg) sqlite3_free(err_msg);
    
    return result == SQLITE_OK;
}

std::string SQLiteStorage::GetCreateTableSQL(const TableData& table) {
    std::ostringstream sql;
    sql << "CREATE TABLE \"" << table.table_name << "\" (";
    sql << "\"_id\" INTEGER PRIMARY KEY AUTOINCREMENT, ";
    
    for (size_t i = 0; i < table.fields.size(); ++i) {
        const auto& field = table.fields[i];
        sql << "\"" << field.name << "\" " << FieldTypeToSQLite(field.type);
        
        if (i < table.fields.size() - 1) {
            sql << ", ";
        }
    }
    
    sql << ", \"_binary\" BLOB";
    sql << ");";
    
    return sql.str();
}

std::string SQLiteStorage::GetInsertSQL(const TableData& table) {
    std::ostringstream sql;
    sql << "INSERT INTO \"" << table.table_name << "\" (";
    
    for (size_t i = 0; i < table.fields.size(); ++i) {
        sql << "\"" << table.fields[i].name << "\"";
        if (i < table.fields.size() - 1) {
            sql << ", ";
        }
    }
    
    sql << ", \"_binary\") VALUES (";
    
    for (size_t i = 0; i <= table.fields.size(); ++i) {
        sql << "?";
        if (i < table.fields.size()) {
            sql << ", ";
        }
    }
    
    sql << ");";
    
    return sql.str();
}

std::string SQLiteStorage::FieldTypeToSQLite(FieldType type) {
    switch (type) {
        case FieldType::INT32:
        case FieldType::INT64:
        case FieldType::UINT32:
        case FieldType::UINT64:
            return "INTEGER";
        case FieldType::FLOAT:
        case FieldType::DOUBLE:
            return "REAL";
        case FieldType::BOOL:
            return "INTEGER";
        case FieldType::STRING:
        case FieldType::BYTES:
        default:
            return "TEXT";
    }
}

} 
