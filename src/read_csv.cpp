#include "k.h"
#include "type_map.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <unordered_map>
#include "inline_query.h"
#include <numeric>
#include "read_csv.h"
#include <ctime>
#include <iomanip>

/**
 * @brief Splits a string into tokens based on a delimiter
 *
 * @param line String to split
 * @param delimiter Character to split on
 * @return std::vector<std::string> Vector of tokens
 */
std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

/**
 * @brief Parses a CSV file and extracts headers and data
 *
 * Handles quoted fields and validates key column if specified.
 * Only reads up to sample_rows for type inference.
 *
 * @param filename Path to CSV file
 * @param delimiter Field separator character
 * @param header Whether first row contains headers
 * @param headers Vector to store column headers
 * @param data Vector to store data rows
 * @param key_column Name of key column (if any)
 * @param sample_rows Number of rows to read for sampling
 * @return bool True if parsing successful, false otherwise
 */
bool parse_csv(const std::string& filename,
               char delimiter,
               bool header,
               std::vector<std::string>& headers,
               std::vector<std::vector<std::string>>& data,
               const std::string& key_column,
               size_t sample_rows = 5) {
    // Open file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return false;
    }

    std::string line;
    bool first_line = true;
    size_t rows_read = 0;

    // Process file line by line
    while (std::getline(file, line) && (rows_read < sample_rows || first_line)) {
        if (line.empty()) continue;
        
        // Parse CSV line handling quoted fields
        std::vector<std::string> row;
        bool in_quotes = false;
        std::string current_field;
        
        for (char c : line) {
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == delimiter && !in_quotes) {
                row.push_back(current_field);
                current_field.clear();
            } else {
                current_field += c;
            }
        }
        row.push_back(current_field);  // Add final field

        if (first_line) {
            if (header) {
                // Process header row
                headers = row;
                if (!key_column.empty()) {
                    // Validate key column exists
                    auto it = std::find(headers.begin(), headers.end(), key_column);
                    if (it == headers.end()) {
                        std::cerr << "Error: Key column '" << key_column
                                  << "' not found in CSV headers." << std::endl;
                        return false;
                    }
                }
                
                // Display headers
                for (const auto& h : headers) {
                    std::cout << "[" << h << "] ";
                }
                std::cout << std::endl;
            } else {
                // Generate default headers for headerless CSV
                headers.clear();
                for (size_t i = 0; i < row.size(); ++i) {
                    headers.push_back("col" + std::to_string(i + 1));
                }
                data.push_back(row);
                rows_read++;
            }
            first_line = false;
        } else {
            data.push_back(row);
            rows_read++;
        }
    }
    file.close();

    if (data.empty()) {
        std::cerr << "Error: No data rows found in CSV file for type inference." << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Infers data types for all columns based on sample data
 *
 * @param headers Vector of column headers
 * @param sample_data Vector of sample data rows
 * @return std::vector<I> Vector of inferred KDB+ type codes
 */
std::vector<I> infer_column_types(const std::vector<std::string>& headers,
                                  const std::vector<std::vector<std::string>>& sample_data) {
    std::vector<I> col_types;
    const auto& type_map = getExtendedTypeMap();

    for (size_t col = 0; col < headers.size(); ++col) {
        std::vector<std::string> column_data;
        for (const auto& row : sample_data) {
            if (col < row.size()) {
                column_data.push_back(row[col]);
            }
        }

        I inferred_type = infer_column_type(column_data);
        col_types.push_back(inferred_type);

        // Print inferred type
        for (const auto& [key, info] : type_map) {
            if (info.kdb_type == inferred_type) {
                break;
            }
        }
    }

    return col_types;
}

/**
 * @brief Constructs KDB+ table creation command
 *
 * Generates q language command to create a table with specified
 * column names, types, and key column.
 *
 * @param table_name Name of table to create
 * @param filename Path to CSV file
 * @param headers Column names
 * @param col_types Vector of KDB+ type codes
 * @param key_column Name of key column (if any)
 * @param delimiter Field separator character
 * @param header Whether first row contains headers
 * @return std::string KDB+ command string
 */
std::string create_table_cmd(const std::string& table_name,
                             const std::string& filename,
                             const std::vector<std::string>& headers,
                             const std::vector<I>& col_types,
                             const std::string& key_column,
                             char delimiter,
                             bool header) {
    const auto& type_map = getExtendedTypeMap();
    std::string type_string;

    // Map KDB+ type codes to type characters
    for (const auto& type_code : col_types) {
        for (const auto& [key, info] : type_map) {
            if (info.kdb_type == type_code) {
                type_string += info.type_char;
                break;
            }
        }
    }

    std::ostringstream cmd;

    // Remove existing table if present
    cmd << "delete " << table_name << " from `.; ";

    // Create path variable and use hsym for more reliable path handling
    cmd << "path:\"" << filename << "\";";
    
    if (!header) {
        // Build column names for headerless CSV
        std::ostringstream col_names;
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) col_names << ",";
            col_names << "`" << headers[i];
        }

        cmd << table_name << ": (" << col_names.str() << ")xcol (\""
            << type_string << "\"; enlist \"" << delimiter
            << "\") 0: hsym `$path";
    } else {
        // Standard CSV load with header
        cmd << table_name << ": (\"" << type_string << "\"; enlist \""
            << delimiter << "\") 0: hsym `$path";
    }

    // Add key column handling
    if (key_column.empty()) {
        cmd << "; `idx xkey update idx:til count i from `" << table_name;
    } else {
        if (!header) {
            cmd << ")";
        }
        cmd << "; (`" << key_column << ") xkey `" << table_name;
    }

    return cmd.str();
}

/**
 * @brief Main function to read CSV file and load it into KDB+
 *
 * Reads CSV file, infers or uses provided column types,
 * creates KDB+ table, and loads data.
 *
 * @param table_name Name of table to create in KDB+
 * @param filename Path to CSV file
 * @param header Whether first row contains headers
 * @param delimiter Field separator character
 * @param key_column Name of key column (if any)
 * @param column_types Vector of type strings (optional)
 * @return bool True if successful, false otherwise
 */
bool read_csv(const std::string& table_name,
              const std::string& filename,
              bool header,
              char delimiter,
              const std::string& key_column,
              const std::vector<std::string>& column_types) {
    const auto& type_map = getExtendedTypeMap();

    // Validate inputs
    if (filename.empty() || table_name.empty()) {
        std::cerr << "Error: Empty filename or table name." << std::endl;
        return false;
    }

    // Parse CSV structure
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> sample_data;
    if (!parse_csv(filename, delimiter, header, headers, sample_data, key_column, 5)) {
        return false;
    }

    std::vector<I> col_types;

    if (!column_types.empty()) {
        // Use provided type specifications
        if (column_types.size() != headers.size()) {
            std::cerr << "Error: Number of provided types (" << column_types.size()
                      << ") doesn't match number of columns (" << headers.size() << ")" << std::endl;
            return false;
        }
        
        for (size_t i = 0; i < column_types.size(); ++i) {
            const auto& type_key = column_types[i];
            if (type_map.find(type_key) == type_map.end()) {
                std::cerr << "Invalid type specified: " << type_key << std::endl;
                return false;
            }
            
            const auto& info = type_map.at(type_key);
            col_types.push_back(info.kdb_type);
        }
    } else {
        // Infer types from sample data
        col_types = infer_column_types(headers, sample_data);
    }

    // Build the KDB+ command
    std::string cmd = create_table_cmd(table_name, filename, headers, col_types, key_column, delimiter, header);

    // Execute query
    auto result = inline_query(cmd);
    if (!bool(result)) {
        std::cerr << "Error: Failed to load CSV." << std::endl;
        return false;
    }

    std::cout << "Table '" << table_name << "' successfully created and populated." << std::endl;
    return true;
}
