#include "excel_converter/excel_converter.h"
#include <iostream>
#include <string>
#include <filesystem>

void PrintUsage(const char* program_name) {
    std::cout << "Excel to SQLite Converter\n\n";
    std::cout << "Usage: " << program_name << " [options] <input_path>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -i, --input <path>      Input Excel file or directory\n";
    std::cout << "  -o, --output <path>     Output SQLite database path\n";
    std::cout << "  -n, --name-row <n>      Row number for field names (default: 1)\n";
    std::cout << "  -t, --type-row <n>      Row number for field types (default: 2)\n";
    std::cout << "  -c, --comment-row <n>   Row number for comments (default: 3)\n";
    std::cout << "  -d, --data-row <n>      Row number where data starts (default: 4)\n";
    std::cout << "  -v, --verbose           Enable verbose output\n";
    std::cout << "  -q, --quiet             Suppress non-error output\n";
    std::cout << "  -h, --help              Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " -i config.xlsx -o output.db\n";
    std::cout << "  " << program_name << " -i ./excel/ -o ./output/\n";
    std::cout << "  " << program_name << " config.xlsx\n";
}

void PrintResult(const excel_converter::ConvertResult& result) {
    std::cout << "\n========== Conversion Result ==========\n";
    std::cout << "Total sheets: " << result.total_sheets << "\n";
    std::cout << "Successful sheets: " << result.success_sheets << "\n";
    std::cout << "Total rows: " << result.total_rows << "\n";
    std::cout << "Successful rows: " << result.success_rows << "\n";
    
    if (!result.sheet_row_counts.empty()) {
        std::cout << "\nSheets processed:\n";
        for (const auto& [name, count] : result.sheet_row_counts) {
            std::cout << "  - " << name << ": " << count << " rows\n";
        }
    }
    
    if (!result.errors.empty()) {
        std::cout << "\nErrors:\n";
        for (const auto& err : result.errors) {
            std::cout << "  [ERROR] " << err << "\n";
        }
    }
    
    if (!result.warnings.empty()) {
        std::cout << "\nWarnings:\n";
        for (const auto& warn : result.warnings) {
            std::cout << "  [WARN] " << warn << "\n";
        }
    }
    
    std::cout << "========================================\n";
    
    if (result.success) {
        std::cout << "\nConversion completed successfully!\n";
    } else {
        std::cout << "\nConversion failed.\n";
    }
}

int main(int argc, char* argv[]) {
    excel_converter::ConvertOptions options;
    std::string input_path;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                input_path = argv[++i];
            } else {
                std::cerr << "Error: Missing argument for " << arg << "\n";
                return 1;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                options.output_db_path = argv[++i];
            } else {
                std::cerr << "Error: Missing argument for " << arg << "\n";
                return 1;
            }
        } else if (arg == "-n" || arg == "--name-row") {
            if (i + 1 < argc) {
                options.name_row = std::stoi(argv[++i]);
            }
        } else if (arg == "-t" || arg == "--type-row") {
            if (i + 1 < argc) {
                options.type_row = std::stoi(argv[++i]);
            }
        } else if (arg == "-c" || arg == "--comment-row") {
            if (i + 1 < argc) {
                options.comment_row = std::stoi(argv[++i]);
            }
        } else if (arg == "-d" || arg == "--data-row") {
            if (i + 1 < argc) {
                options.data_start_row = std::stoi(argv[++i]);
            }
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            options.verbose = false;
        } else if (arg[0] != '-') {
            if (input_path.empty()) {
                input_path = arg;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    if (input_path.empty()) {
        std::cerr << "Error: No input path specified\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    options.excel_path = input_path;
    
    std::cout << "Excel to SQLite Converter\n";
    std::cout << "Input: " << input_path << "\n";
    if (!options.output_db_path.empty()) {
        std::cout << "Output: " << options.output_db_path << "\n";
    }
    std::cout << "\n";
    
    excel_converter::ExcelConverter converter;
    
    if (!converter.Initialize(options)) {
        std::cerr << "Failed to initialize converter: " << converter.GetLastError() << "\n";
        return 1;
    }
    
    converter.SetProgressCallback([](const std::string& stage, int current, int total) {
        std::cout << "[" << current << "/" << total << "] " << stage << "\n";
    });
    
    auto result = converter.Convert();
    
    PrintResult(result);
    
    return result.success ? 0 : 1;
}
