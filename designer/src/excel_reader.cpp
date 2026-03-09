#include "excel_converter/excel_reader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>

#ifdef USE_XLSXIO
#include <xlsxio_read.h>
#endif

namespace excel_converter {

struct ExcelReader::Impl {
#ifdef USE_XLSXIO
    xlsxioreader handle = nullptr;
#endif
    std::vector<std::string> sheet_names;
    std::map<std::string, TableData> cached_tables;
};

ExcelReader::ExcelReader() 
    : impl_(std::make_unique<Impl>())
    , is_csv_(false) {
}

ExcelReader::~ExcelReader() {
    Close();
}

bool ExcelReader::Open(const std::string& file_path) {
    Close();
    
    current_file_ = file_path;
    is_csv_ = IsCSVFile(file_path);
    
    if (is_csv_) {
        return true;
    }
    
#ifdef USE_XLSXIO
    impl_->handle = xlsxioread_open(file_path.c_str());
    if (!impl_->handle) {
        last_error_ = "Failed to open Excel file: " + file_path;
        return false;
    }
    
    xlsxioreadersheetlist sheet_list = xlsxioread_sheetlist_open(impl_->handle);
    if (sheet_list) {
        const char* name = nullptr;
        while ((name = xlsxioread_sheetlist_next(sheet_list)) != nullptr) {
            impl_->sheet_names.push_back(name);
        }
        xlsxioread_sheetlist_close(sheet_list);
    }
    
    return true;
#else
    last_error_ = "Excel file support not compiled. Please use CSV files or compile with XLSXIO support.";
    return false;
#endif
}

void ExcelReader::Close() {
#ifdef USE_XLSXIO
    if (impl_->handle) {
        xlsxioread_close(impl_->handle);
        impl_->handle = nullptr;
    }
#endif
    impl_->sheet_names.clear();
    impl_->cached_tables.clear();
    current_file_.clear();
    is_csv_ = false;
}

bool ExcelReader::IsCSVFile(const std::string& file_path) {
    std::string ext = file_path;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext.size() >= 4 && ext.substr(ext.size() - 4) == ".csv";
}

std::vector<std::string> ExcelReader::ParseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            result.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    
    result.push_back(field);
    return result;
}

bool ExcelReader::ReadCSV(const std::string& file_path, std::vector<std::vector<std::string>>& data) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        last_error_ = "Failed to open CSV file: " + file_path;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (!line.empty()) {
            data.push_back(ParseCSVLine(line));
        }
    }
    
    return true;
}

bool ExcelReader::ReadExcel(const std::string& file_path, std::map<std::string, TableData>& out_tables) {
#ifdef USE_XLSXIO
    for (const auto& sheet_name : impl_->sheet_names) {
        TableData data;
        if (ReadSheet(sheet_name, data)) {
            out_tables[sheet_name] = std::move(data);
        }
    }
    return !out_tables.empty();
#else
    last_error_ = "Excel file support not compiled";
    return false;
#endif
}

std::vector<std::string> ExcelReader::GetSheetNames() {
    if (is_csv_) {
        std::filesystem::path p(current_file_);
        return { p.stem().string() };
    }
    
    return impl_->sheet_names;
}

bool ExcelReader::ReadSheet(const std::string& sheet_name, TableData& out_data) {
    if (is_csv_) {
        std::vector<std::vector<std::string>> raw_data;
        if (!ReadCSV(current_file_, raw_data)) {
            return false;
        }
        
        if (raw_data.size() < 4) {
            last_error_ = "CSV file has insufficient rows (need at least 4 for header)";
            return false;
        }
        
        std::filesystem::path p(current_file_);
        out_data.table_name = p.stem().string();
        
        if (!ParseHeader(raw_data, out_data, 0, 1, 2)) {
            return false;
        }
        
        for (size_t i = 3; i < raw_data.size(); ++i) {
            if (raw_data[i].empty() || (raw_data[i].size() == 1 && raw_data[i][0].empty())) {
                continue;
            }
            out_data.rows.push_back(raw_data[i]);
        }
        
        return true;
    }
    
#ifdef USE_XLSXIO
    if (!impl_->handle) {
        last_error_ = "No Excel file opened";
        return false;
    }
    
    xlsxioreadersheet sheet = xlsxioread_sheet_open(impl_->handle, sheet_name.c_str(), XLSXIOREAD_SKIP_EMPTY_ROWS);
    if (!sheet) {
        last_error_ = "Failed to open sheet: " + sheet_name;
        return false;
    }
    
    std::vector<std::vector<std::string>> raw_data;
    std::vector<std::string> row_data;
    
    while (xlsxioread_sheet_next_row(sheet)) {
        row_data.clear();
        char* value = nullptr;
        while ((value = xlsxioread_sheet_next_cell(sheet)) != nullptr) {
            row_data.push_back(value ? value : "");
            if (value) {
                xlsxioread_free(value);
            }
        }
        raw_data.push_back(row_data);
    }
    
    xlsxioread_sheet_close(sheet);
    
    if (raw_data.size() < 4) {
        last_error_ = "Sheet " + sheet_name + " has insufficient rows (need at least 4 for header)";
        return false;
    }
    
    out_data.table_name = sheet_name;
    
    if (!ParseHeader(raw_data, out_data, 0, 1, 2)) {
        return false;
    }
    
    for (size_t i = 3; i < raw_data.size(); ++i) {
        if (raw_data[i].empty() || (raw_data[i].size() == 1 && raw_data[i][0].empty())) {
            continue;
        }
        out_data.rows.push_back(raw_data[i]);
    }
    
    return true;
#else
    last_error_ = "Excel file support not compiled";
    return false;
#endif
}

bool ExcelReader::ReadAllSheets(std::map<std::string, TableData>& out_tables) {
    if (is_csv_) {
        TableData data;
        if (ReadSheet("", data)) {
            std::filesystem::path p(current_file_);
            out_tables[p.stem().string()] = std::move(data);
        }
        return !out_tables.empty();
    }
    
    return ReadExcel(current_file_, out_tables);
}

bool ExcelReader::ParseHeader(const std::vector<std::vector<std::string>>& raw_data,
                              TableData& out_data,
                              int name_row,
                              int type_row,
                              int comment_row) {
    if (raw_data.size() <= static_cast<size_t>(std::max({name_row, type_row, comment_row}))) {
        last_error_ = "Insufficient rows for header parsing";
        return false;
    }
    
    const auto& names = raw_data[name_row];
    const auto& types = raw_data[type_row];
    const auto& comments = (comment_row >= 0 && comment_row < static_cast<int>(raw_data.size())) 
                           ? raw_data[comment_row] : std::vector<std::string>();
    
    size_t col_count = names.size();
    
    for (size_t i = 0; i < col_count; ++i) {
        std::string name = TrimString(names[i]);
        if (name.empty()) {
            continue;
        }
        
        FieldInfo field;
        field.name = name;
        field.column_index = static_cast<int>(i);
        
        if (i < types.size()) {
            std::string type_str = TrimString(types[i]);
            
            if (type_str.size() > 2 && type_str.back() == ']' && type_str[type_str.size() - 2] == '[') {
                field.is_array = true;
                type_str = type_str.substr(0, type_str.size() - 2);
            } else {
                field.is_array = false;
            }
            
            field.type = ParseFieldType(type_str);
            field.type_name = type_str;
        } else {
            field.type = FieldType::STRING;
            field.type_name = "string";
            field.is_array = false;
        }
        
        if (i < comments.size()) {
            field.comment = TrimString(comments[i]);
        }
        
        out_data.fields.push_back(field);
    }
    
    return !out_data.fields.empty();
}

FieldType ExcelReader::ParseFieldType(const std::string& type_str) {
    std::string lower = type_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "int32" || lower == "int" || lower == "i32") {
        return FieldType::INT32;
    } else if (lower == "int64" || lower == "long" || lower == "i64") {
        return FieldType::INT64;
    } else if (lower == "uint32" || lower == "u32") {
        return FieldType::UINT32;
    } else if (lower == "uint64" || lower == "u64") {
        return FieldType::UINT64;
    } else if (lower == "float" || lower == "f32") {
        return FieldType::FLOAT;
    } else if (lower == "double" || lower == "f64") {
        return FieldType::DOUBLE;
    } else if (lower == "string" || lower == "str" || lower == "text") {
        return FieldType::STRING;
    } else if (lower == "bool" || lower == "boolean") {
        return FieldType::BOOL;
    } else if (lower == "bytes" || lower == "binary" || lower == "blob") {
        return FieldType::BYTES;
    }
    
    return FieldType::UNKNOWN;
}

std::string ExcelReader::TrimString(const std::string& str) {
    size_t start = 0;
    size_t end = str.size();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

} 
