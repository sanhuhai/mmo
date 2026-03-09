#include "excel_converter/excel_converter.h"
#include <filesystem>
#include <iostream>

namespace excel_converter {

ExcelConverter::ExcelConverter() 
    : reader_(std::make_unique<ExcelReader>())
    , storage_(std::make_unique<SQLiteStorage>()) {
}

ExcelConverter::~ExcelConverter() = default;

bool ExcelConverter::Initialize(const ConvertOptions& options) {
    options_ = options;
    return true;
}

ConvertResult ExcelConverter::Convert() {
    ConvertResult result;
    
    if (options_.excel_path.empty()) {
        result.errors.push_back("Excel path not specified");
        return result;
    }
    
    std::filesystem::path path(options_.excel_path);
    
    if (std::filesystem::is_directory(path)) {
        return ConvertDirectory(options_.excel_path);
    } else {
        return ConvertFile(options_.excel_path);
    }
}

ConvertResult ExcelConverter::ConvertFile(const std::string& excel_path) {
    ConvertResult result;
    
    if (!reader_->Open(excel_path)) {
        result.errors.push_back("Failed to open Excel file: " + excel_path + " - " + reader_->GetLastError());
        return result;
    }
    
    std::filesystem::path excel_file(excel_path);
    std::string db_path = options_.output_db_path;
    
    if (db_path.empty()) {
        db_path = (excel_file.parent_path() / excel_file.stem()).string() + ".db";
    }
    
    if (!storage_->Open(db_path)) {
        result.errors.push_back("Failed to open SQLite database: " + db_path + " - " + storage_->GetLastError());
        return result;
    }
    
    std::map<std::string, TableData> tables;
    if (!reader_->ReadAllSheets(tables)) {
        result.errors.push_back("Failed to read sheets from Excel file");
        return result;
    }
    
    result.total_sheets = static_cast<int>(tables.size());
    
    int current = 0;
    for (auto& [name, table] : tables) {
        ++current;
        
        if (progress_callback_) {
            progress_callback_("Processing sheet: " + name, current, result.total_sheets);
        }
        
        if (!ValidateTable(table)) {
            result.warnings.push_back("Skipped invalid table: " + name);
            continue;
        }
        
        if (ProcessSheet(table, result)) {
            ++result.success_sheets;
            result.sheet_row_counts[name] = static_cast<int>(table.rows.size());
        }
    }
    
    result.success = (result.success_sheets > 0);
    return result;
}

ConvertResult ExcelConverter::ConvertDirectory(const std::string& directory) {
    ConvertResult result;
    
    std::filesystem::path dir_path(directory);
    
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        result.errors.push_back("Invalid directory: " + directory);
        return result;
    }
    
    std::vector<std::string> excel_files;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".xlsx" || ext == ".xls" || ext == ".csv") {
                excel_files.push_back(entry.path().string());
            }
        }
    }
    
    if (excel_files.empty()) {
        result.errors.push_back("No Excel or CSV files found in directory: " + directory);
        return result;
    }
    
    std::filesystem::path output_dir = options_.output_db_path.empty() 
        ? dir_path / "output" 
        : std::filesystem::path(options_.output_db_path);
    
    std::filesystem::create_directories(output_dir);
    
    for (const auto& file : excel_files) {
        if (options_.verbose) {
            std::cout << "Processing: " << file << std::endl;
        }
        
        std::filesystem::path excel_file(file);
        std::string db_path = (output_dir / excel_file.stem()).string() + ".db";
        
        reader_ = std::make_unique<ExcelReader>();
        storage_ = std::make_unique<SQLiteStorage>();
        
        if (!reader_->Open(file)) {
            result.errors.push_back("Failed to open file: " + file);
            continue;
        }
        
        if (!storage_->Open(db_path)) {
            result.errors.push_back("Failed to open SQLite database: " + db_path);
            continue;
        }
        
        std::map<std::string, TableData> tables;
        if (!reader_->ReadAllSheets(tables)) {
            result.errors.push_back("Failed to read sheets from file: " + file);
            continue;
        }
        
        result.total_sheets += static_cast<int>(tables.size());
        
        for (auto& [name, table] : tables) {
            if (progress_callback_) {
                progress_callback_("Processing sheet: " + name, 
                                  static_cast<int>(result.success_sheets + 1), 
                                  static_cast<int>(tables.size()));
            }
            
            if (!ValidateTable(table)) {
                result.warnings.push_back("Skipped invalid table: " + name);
                continue;
            }
            
            if (ProcessSheet(table, result)) {
                ++result.success_sheets;
                result.sheet_row_counts[name] = static_cast<int>(table.rows.size());
            }
        }
    }
    
    result.success = (result.success_sheets > 0);
    return result;
}

bool ExcelConverter::ProcessSheet(const TableData& table, ConvertResult& result) {
    if (!storage_->CreateTable(table)) {
        result.errors.push_back("Failed to create table " + table.table_name + ": " + storage_->GetLastError());
        return false;
    }
    
    std::vector<BinaryData> binary_rows;
    if (!BinarySerializer::SerializeTable(table, binary_rows)) {
        result.errors.push_back("Failed to serialize table " + table.table_name);
        return false;
    }
    
    result.total_rows += static_cast<int>(table.rows.size());
    
    if (!storage_->InsertTable(table, binary_rows)) {
        result.errors.push_back("Failed to insert data into table " + table.table_name + ": " + storage_->GetLastError());
        return false;
    }
    
    result.success_rows += static_cast<int>(table.rows.size());
    
    if (options_.verbose) {
        Log("Converted table " + table.table_name + " with " + std::to_string(table.rows.size()) + " rows");
    }
    
    return true;
}

bool ExcelConverter::ValidateTable(const TableData& table) {
    if (table.table_name.empty()) {
        return false;
    }
    
    if (table.fields.empty()) {
        return false;
    }
    
    for (const auto& field : table.fields) {
        if (field.name.empty()) {
            return false;
        }
    }
    
    return true;
}

void ExcelConverter::Log(const std::string& message, bool is_error) {
    if (is_error) {
        std::cerr << "[ERROR] " << message << std::endl;
    } else if (options_.verbose) {
        std::cout << "[INFO] " << message << std::endl;
    }
}

} 
