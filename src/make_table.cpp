#include "make_table.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "inline_query.h"

/**
 * @brief Creates a KDB+ table with the specified name, columns, and data
 *
 * @param table_name Name of the table to create (must be a valid KDB+ identifier)
 * @param column_names Vector of column names (must be valid KDB+ identifiers)
 * @param data 2D vector where data[i][j] is the value for row i, column j
 * @return bool True if table creation succeeds, false otherwise
 * @throws Does not throw exceptions, failures are returned as false
 *
 * @note Column names should not contain spaces or special characters
 * @note String values containing backticks (`) are automatically escaped
 * @note Numeric values maintain precision up to 15 decimal places
 */
bool make_table(const std::string& table_name,
               const std::vector<std::string>& column_names,
               const std::vector<std::vector<KDBType>>& data) {
    // Check if column names or data is empty and log an error if so
    if (column_names.empty() || data.empty()) {
        std::cerr << "Column names or data is empty" << std::endl;
        return false;
    }

    size_t num_columns = column_names.size(); ///< Number of columns in the table
    size_t num_rows = data.size();            ///< Number of rows in the table

    // Ensure that each row has the correct number of columns
    for (size_t row = 0; row < num_rows; ++row) {
        if (data[row].size() != num_columns) {
            std::cerr << "Row " << row << " does not have the correct number of columns" << std::endl;
            return false;
        }
    }

    // Build the q-sql command string to create the table
    std::ostringstream q_command;
    q_command << table_name << ": ([] "; ///< Start constructing the table with column definitions

    // Iterate over each column to define its data
    for (size_t col = 0; col < num_columns; ++col) {
        if (col > 0) q_command << "; "; ///< Separate columns with a semicolon
        q_command << column_names[col] << ":"; ///< Define the column name

        // Handle single-row data with special formatting
        if (num_rows == 1) {
            const KDBType& value = data[0][col]; ///< Retrieve the value for the single row
            if (std::holds_alternative<std::monostate>(value)) {
                q_command << "enlist ::"; ///< Represent null values
            } else if (std::holds_alternative<bool>(value)) {
                q_command << "enlist " << (std::get<bool>(value) ? "1b" : "0b"); ///< Represent boolean values
            } else if (std::holds_alternative<int>(value)) {
                q_command << "enlist " << std::get<int>(value); ///< Represent integer values
            } else if (std::holds_alternative<double>(value)) {
                q_command << "enlist " << std::setprecision(15) << std::get<double>(value); ///< Represent double values with high precision
            } else if (std::holds_alternative<std::string>(value)) {
                std::string str = std::get<std::string>(value);
                q_command << "enlist `" << str; ///< Represent string values as symbols
            }
        } else {
            // Handle multiple rows by constructing a list of values for the column
            q_command << "(";
            for (size_t row = 0; row < num_rows; ++row) {
                if (row > 0) q_command << ";"; ///< Separate values with a semicolon
                const KDBType& value = data[row][col]; ///< Retrieve the value for the current row

                if (std::holds_alternative<std::monostate>(value)) {
                    q_command << "::"; ///< Represent null values
                } else if (std::holds_alternative<bool>(value)) {
                    q_command << (std::get<bool>(value) ? "1b" : "0b"); ///< Represent boolean values
                } else if (std::holds_alternative<int>(value)) {
                    q_command << std::get<int>(value); ///< Represent integer values
                } else if (std::holds_alternative<double>(value)) {
                    q_command << std::setprecision(15) << std::get<double>(value); ///< Represent double values with high precision
                } else if (std::holds_alternative<std::string>(value)) {
                    std::string str = std::get<std::string>(value);
                    // Escape backticks in strings to prevent syntax errors in q-sql
                    size_t pos = 0;
                    while ((pos = str.find('`', pos)) != std::string::npos) {
                        str.insert(pos, "`");
                        pos += 2; ///< Move past the escaped backtick
                    }
                    q_command << "`" << str; ///< Represent string values as escaped symbols
                }
            }
            q_command << ")"; ///< Close the list of values for the column
        }
    }

    q_command << ")"; ///< Close the table definition

    // Execute the constructed q-sql command using the inline_query function
    auto result = inline_query(q_command.str());
    return bool(result); ///< Return the success status of the table creation
}
