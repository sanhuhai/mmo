#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace excel_converter {

enum class FieldType {
    INT32,
    INT64,
    UINT32,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,
    BOOL,
    BYTES,
    UNKNOWN
};

struct FieldInfo {
    std::string name;
    FieldType type;
    std::string type_name;
    int column_index;
    bool is_array;
    std::string comment;
};

struct TableData {
    std::string table_name;
    std::vector<FieldInfo> fields;
    std::vector<std::vector<std::string>> rows;
    std::string comment;
};

class ExcelReader {
public:
    ExcelReader();
    ~ExcelReader();

    bool Open(const std::string& file_path);
    void Close();

    std::vector<std::string> GetSheetNames();
    bool ReadSheet(const std::string& sheet_name, TableData& out_data);

    bool ReadAllSheets(std::map<std::string, TableData>& out_tables);

    const std::string& GetLastError() const { return last_error_; }

private:
    bool ParseHeader(const std::vector<std::vector<std::string>>& raw_data, 
                     TableData& out_data, 
                     int name_row, 
                     int type_row, 
                     int comment_row);
    
    FieldType ParseFieldType(const std::string& type_str);
    
    std::string TrimString(const std::string& str);
    
    std::vector<std::string> ParseCSVLine(const std::string& line);
    bool IsCSVFile(const std::string& file_path);
    bool ReadCSV(const std::string& file_path, std::vector<std::vector<std::string>>& data);
    bool ReadExcel(const std::string& file_path, std::map<std::string, TableData>& out_tables);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string last_error_;
    std::string current_file_;
    bool is_csv_;
};

} 
