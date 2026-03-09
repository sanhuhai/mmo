#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "excel_reader.h"
#include "binary_serializer.h"

namespace excel_converter {

struct ConvertOptions {
    std::string excel_path;
    std::string output_db_path;
    std::string output_bin_path;
    int name_row = 1;
    int type_row = 2;
    int comment_row = 3;
    int data_start_row = 4;
    bool skip_empty_rows = true;
    bool create_binary_files = true;
    bool verbose = true;
};

struct ConvertResult {
    bool success = false;
    int total_sheets = 0;
    int success_sheets = 0;
    int total_rows = 0;
    int success_rows = 0;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, int> sheet_row_counts;
};

class ExcelConverter {
public:
    using ProgressCallback = std::function<void(const std::string& stage, int current, int total)>;

    ExcelConverter();
    ~ExcelConverter();

    bool Initialize(const ConvertOptions& options);
    
    ConvertResult Convert();
    ConvertResult ConvertFile(const std::string& excel_path);
    ConvertResult ConvertDirectory(const std::string& directory);

    void SetProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }

    const std::string& GetLastError() const { return last_error_; }

private:
    bool ProcessSheet(const TableData& table, ConvertResult& result);
    bool ValidateTable(const TableData& table);
    void Log(const std::string& message, bool is_error = false);

private:
    ConvertOptions options_;
    std::unique_ptr<ExcelReader> reader_;
    std::unique_ptr<SQLiteStorage> storage_;
    ProgressCallback progress_callback_;
    std::string last_error_;
};

} 
