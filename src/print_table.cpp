#include "print_table.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <iomanip>
#include <numeric>
#include "type_map.h"

namespace {  // Anonymous namespace for internal helper functions

/**
 * @brief Converts KDB+ type code to human-readable string
 *
 * @param coldata K object containing column data
 * @return std::string Human-readable type name
 */
std::string get_type_string(K coldata) {
    if (!coldata) return "null";
    
    switch (coldata->t) {
        case -KB: case KB: return "bool";      // Boolean type
        case -KG: case KG: return "byte";      // Byte type
        case -KH: case KH: return "short";     // Short integer
        case -KI: case KI: return "int";       // Integer
        case -KJ: case KJ: return "long";      // Long integer
        case -KE: case KE: return "real";      // Real number
        case -KF: case KF: return "float";     // Float
        case -KC: case KC: return "char";      // Character
        case -KS: case KS: return "sym";       // Symbol
        case -KM: case KM: return "month";     // Month
        case -KD: case KD: return "date";      // Date
        case -KZ: case KZ: return "datetime";  // Datetime
        case -KU: case KU: return "minute";    // Minute
        case -KV: case KV: return "second";    // Second
        case -KT: case KT: return "time";      // Time
        case -KP: case KP: return "timestamp"; // Timestamp
        case -KN: case KN: return "timespan";  // Timespan
        default: return "unknown";             // Unknown type
    }
}

/**
 * @brief Calculates appropriate column widths for table display
 *
 * Determines the maximum width needed for each column based on:
 * - Column name length
 * - Type name length
 * - Maximum value length (truncated to 30 characters)
 */
std::vector<size_t> calculate_widths(K table) {
    std::vector<size_t> widths;
    
    if (table->t == XT) {
        K dict = table->k;
        K colnames = kK(dict)[0];
        K colvalues = kK(dict)[1];
        
        // Initialize widths based on column names and types
        for (J i = 0; i < colnames->n; ++i) {
            std::string colname = kS(colnames)[i];
            widths.push_back(std::max(colname.length(),
                           get_type_string(kK(colvalues)[i]).length()));
        }
        
        // Update widths based on actual values
        J row_count = kK(colvalues)[0]->n;
        for (J col = 0; col < colvalues->n; ++col) {
            K coldata = kK(colvalues)[col];
            for (J row = 0; row < row_count; ++row) {
                std::string val = format_value(coldata, row);
                if (val.length() > 30) val = val.substr(0, 27) + "...";
                widths[col] = std::max(widths[col], val.length());
            }
        }
    }
    
    return widths;
}

/**
 * @brief Prints a horizontal separator line
 */
void print_separator_table(const std::vector<size_t>& widths) {
    size_t total_width = std::accumulate(widths.begin(), widths.end(), 0) +
                        (widths.size() * 3) + 1;  // Account for borders and padding
    std::cout << std::string(total_width, '-') << std::endl;
}

/**
 * @brief Prints table header including column types and names
 */
void print_table_header(K table, const std::vector<size_t>& widths) {
    if (!table || table->t == -128) return;

    if (table->t == XT) {
        K dict = table->k;
        K colnames = kK(dict)[0];
        K colvalues = kK(dict)[1];

        // Print column types row
        for (J i = 0; i < colnames->n; ++i) {
            std::cout << "| " << std::left << std::setw(widths[i])
                     << get_type_string(kK(colvalues)[i]) << " ";
        }
        std::cout << "|" << std::endl;

        print_separator_table(widths);

        // Print column names row
        for (J i = 0; i < colnames->n; ++i) {
            std::cout << "| " << std::left << std::setw(widths[i])
                     << kS(colnames)[i] << " ";
        }
        std::cout << "|" << std::endl;
    }
}

/**
 * @brief Prints a single table row
 */
void print_row(K table, J row_idx, const std::vector<size_t>& widths) {
    if (table->t == XT) {
        K dict = table->k;
        K colvalues = kK(dict)[1];

        for (J col = 0; col < colvalues->n; ++col) {
            K coldata = kK(colvalues)[col];
            std::string val = format_value(coldata, row_idx);
            if (val.length() > 30) val = val.substr(0, 27) + "...";
            std::cout << "| " << std::left << std::setw(widths[col]) << val << " ";
        }
        std::cout << "|" << std::endl;
    }
}

} // end anonymous namespace

/**
 * @brief Prints the first n rows of a table
 *
 * Displays table metadata, column types, names, and the first n rows
 * of data in a formatted table layout.
 *
 * @param table K object containing table data
 * @param n Number of rows to print (default: 5)
 */
void print_head(K table, int n) {
    if (!table || table->t == -128) return;

    if (table->t == XT) {
        auto widths = calculate_widths(table);
        K dict = table->k;
        K colvalues = kK(dict)[1];
        J row_count = kK(colvalues)[0]->n;
        n = std::min((J)n, row_count);

        // Print table metadata
        std::cout << "Table Head [" << n << " of " << row_count << " rows × "
                 << colvalues->n << " columns]:" << std::endl;
        
        // Print formatted table
        print_separator_table(widths);
        print_table_header(table, widths);
        print_separator_table(widths);

        for (J row = 0; row < n; ++row) {
            print_row(table, row, widths);
        }
        print_separator_table(widths);
    }
}

/**
 * @brief Prints the last n rows of a table
 *
 * Displays table metadata, column types, names, and the last n rows
 * of data in a formatted table layout.
 *
 * @param table K object containing table data
 * @param n Number of rows to print (default: 5)
 */
void print_tail(K table, int n) {
    if (!table || table->t == -128) return;

    if (table->t == XT) {
        auto widths = calculate_widths(table);
        K dict = table->k;
        K colvalues = kK(dict)[1];
        J row_count = kK(colvalues)[0]->n;
        n = std::min((J)n, row_count);

        // Print table metadata
        std::cout << "Table Tail [last " << n << " of " << row_count << " rows × "
                 << colvalues->n << " columns]:" << std::endl;
        
        // Print formatted table
        print_separator_table(widths);
        print_table_header(table, widths);
        print_separator_table(widths);

        for (J row = row_count - n; row < row_count; ++row) {
            print_row(table, row, widths);
        }
        print_separator_table(widths);
    }
}
