#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include "excel_reader.h"

struct sqlite3;
struct sqlite3_stmt;

namespace excel_converter {

struct BinaryData {
    std::vector<uint8_t> data;
    
    void WriteInt32(int32_t value);
    void WriteUInt32(uint32_t value);
    void WriteInt64(int64_t value);
    void WriteUInt64(uint64_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(const std::string& value);
    void WriteBool(bool value);
    void WriteBytes(const std::vector<uint8_t>& value);
    void WriteLength(uint32_t length);
    
    int32_t ReadInt32(size_t& offset) const;
    uint32_t ReadUInt32(size_t& offset) const;
    int64_t ReadInt64(size_t& offset) const;
    uint64_t ReadUInt64(size_t& offset) const;
    float ReadFloat(size_t& offset) const;
    double ReadDouble(size_t& offset) const;
    std::string ReadString(size_t& offset) const;
    bool ReadBool(size_t& offset) const;
    std::vector<uint8_t> ReadBytes(size_t& offset) const;
};

class BinarySerializer {
public:
    static bool SerializeRow(const TableData& table, 
                            const std::vector<std::string>& row,
                            BinaryData& out_data);
    
    static bool SerializeTable(const TableData& table, 
                              std::vector<BinaryData>& out_rows);
    
    static bool SerializeField(const FieldInfo& field,
                              const std::string& value,
                              BinaryData& out_data);

private:
    static bool ParseValue(const std::string& value, FieldType type, BinaryData& out_data);
    static std::vector<std::string> ParseArrayValue(const std::string& value);
};

class SQLiteStorage {
public:
    SQLiteStorage();
    ~SQLiteStorage();

    bool Open(const std::string& db_path);
    void Close();

    bool CreateTable(const TableData& table);
    bool InsertRow(const TableData& table, 
                   const std::vector<std::string>& row,
                   const BinaryData& binary_data);
    bool InsertTable(const TableData& table, 
                    const std::vector<BinaryData>& binary_rows);

    bool TableExists(const std::string& table_name);
    bool DropTable(const std::string& table_name);
    bool ClearTable(const std::string& table_name);

    bool BeginTransaction();
    bool CommitTransaction();
    bool RollbackTransaction();

    const std::string& GetLastError() const { return last_error_; }

private:
    std::string GetCreateTableSQL(const TableData& table);
    std::string GetInsertSQL(const TableData& table);
    std::string FieldTypeToSQLite(FieldType type);

private:
    sqlite3* db_;
    std::string last_error_;
};

} 
