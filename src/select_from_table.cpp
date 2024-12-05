
#include "select_from_table.h"
#include "type_map.h"
#include "inline_query.h"
#include <sstream>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <iomanip>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

namespace kdb_utils {

/**
 * @class KDBValueConverter
 * @brief Responsible for converting KDB+ data types into KDBValue objects.
 *
 * Provides static methods to convert various KDB+ data structures (atoms, vectors, general lists)
 * into a unified KDBValue representation for further processing in C++.
 */
class KDBValueConverter {
public:
    /**
     * @brief Converts a KDB+ object to a KDBValue.
     *
     * This function determines the type of the KDB+ object and dispatches
     * to the appropriate helper function to handle atoms, vectors, or general lists.
     *
     * @param data Pointer to the KDB+ object (K structure).
     * @param idx Index to access in case of vector data types (default is 0).
     * @return KDBValue The converted value.
     */
    static KDBValue convert_k_to_value(K data, size_t idx = 0) {
        // Check if data is null.
        if (!data) return KDBValue();
        // Dispatch based on the type code.
        if (data->t < 0) {
            // Atom (scalar value).
            return convert_atom(data);
        } else if (data->t > 0) {
            // Vector (list of values).
            return convert_vector(data, idx);
        } else if (data->t == 0) {
            // General list (mixed types).
            return convert_general_list(data, idx);
        }

        // If type is unhandled, output a message.
        std::cout << "Unhandled type code: " << static_cast<int>(data->t) << std::endl;
        return KDBValue();
    }

private:
    /**
     * @brief Converts an atom (scalar) KDB+ object to KDBValue.
     *
     * Handles various KDB+ atomic types by mapping them to corresponding C++ types.
     *
     * @param data Pointer to the KDB+ atom.
     * @return KDBValue The converted value.
     */
    static KDBValue convert_atom(K data) {
        switch (data->t) {
            case -KB: return KDBValue(static_cast<bool>(data->g));                   // Boolean
            case -KG: return KDBValue(static_cast<unsigned char>(data->g));          // Byte
            case -KH: return KDBValue(data->h);                                      // Short
            case -KI: return KDBValue(data->i);                                      // Int
            case -KJ: return KDBValue(data->j);                                      // Long
            case -KE: return KDBValue(static_cast<float>(data->e));                  // Real
            case -KF: return KDBValue(data->f);                                      // Float
            case -KC: return KDBValue(static_cast<char>(data->g));                   // Char
            case -KS: {                                                              // Symbol
                if (data->s) {
                    std::string val(data->s);
                    //std::cout << "Got symbol atom value: '" << val << "'" << std::endl;
                    return KDBValue(val);
                }
                return KDBValue();
            }
            // Date and time types.
            case -KM: return KDBValue::create_month(data->i);                        // Month
            case -KD: return KDBValue::create_date(data->i);                         // Date
            case -KU: return KDBValue::create_minute(data->i);                       // Minute
            case -KV: return KDBValue::create_second(data->i);                       // Second
            case -KT: return KDBValue::create_time(data->i);                         // Time
            case -KZ: return KDBValue::create_datetime(data->f);                     // DateTime
            case -KN: return KDBValue::create_timespan(data->j);                     // Timespan
            default:
                std::cout << "Unknown atom type: " << static_cast<int>(data->t) << std::endl;
                return KDBValue();
        }
    }

    /**
     * @brief Converts a vector KDB+ object to KDBValue.
     *
     * Accesses the element at the specified index within the vector and converts it.
     *
     * @param data Pointer to the KDB+ vector.
     * @param idx Index of the element to convert.
     * @return KDBValue The converted value.
     */
    static KDBValue convert_vector(K data, size_t idx) {
        // Check for index out of bounds.
        if (idx >= static_cast<size_t>(data->n)) {
            std::cout << "Index " << idx << " out of bounds for vector of size " << data->n << std::endl;
            return KDBValue();
        }

        // Access and convert the element at idx based on vector type.
        switch (data->t) {
            case KB: return KDBValue(static_cast<bool>(kG(data)[idx]));              // Boolean vector
            case KG: return KDBValue(static_cast<unsigned char>(kG(data)[idx]));     // Byte vector
            case KH: return KDBValue(kH(data)[idx]);                                 // Short vector
            case KI: return KDBValue(kI(data)[idx]);                                 // Int vector
            case KJ: return KDBValue(kJ(data)[idx]);                                 // Long vector
            case KE: return KDBValue(static_cast<float>(kE(data)[idx]));             // Real vector
            case KF: return KDBValue(kF(data)[idx]);                                 // Float vector
            case KC: return KDBValue(kC(data)[idx]);                                 // Char vector
            case KS: {                                                               // Symbol vector
                if (kS(data)) {
                    std::string val(kS(data)[idx]);
                    //std::cout << "Got symbol vector value: '" << val << "'" << std::endl;
                    return KDBValue(val);
                }
                return KDBValue();
            }
            // Date and time types.
            case KM: return KDBValue::create_month(kI(data)[idx]);                   // Month vector
            case KD: return KDBValue::create_date(kI(data)[idx]);                    // Date vector
            case KU: return KDBValue::create_minute(kI(data)[idx]);                  // Minute vector
            case KV: return KDBValue::create_second(kI(data)[idx]);                  // Second vector
            case KT: return KDBValue::create_time(kI(data)[idx]);                    // Time vector
            case KZ: return KDBValue::create_datetime(kF(data)[idx]);                // DateTime vector
            case KN: return KDBValue::create_timespan(kJ(data)[idx]);                // Timespan vector
            default:
                std::cout << "Unknown vector type: " << static_cast<int>(data->t) << std::endl;
                return KDBValue();
        }
    }

    /**
     * @brief Converts a general list KDB+ object to KDBValue.
     *
     * Handles mixed-type lists by accessing the element at the specified index
     * and recursively converting it.
     *
     * @param data Pointer to the KDB+ general list.
     * @param idx Index of the element to convert.
     * @return KDBValue The converted value.
     */
    static KDBValue convert_general_list(K data, size_t idx) {
        // Output the size of the list for debugging.
        std::cout << "Processing general list with " << data->n << " elements" << std::endl;

        // Check for index out of bounds.
        if (idx >= static_cast<size_t>(data->n)) {
            std::cout << "Index " << idx << " out of bounds for list of size " << data->n << std::endl;
            return KDBValue();
        }

        // Access the element and recursively convert it.
        K element = kK(data)[idx];
        return convert_k_to_value(element);
    }
};

/**
 * @class TableProcessor
 * @brief Processes KDB+ table results and converts them into KDBResult objects.
 *
 * Provides methods to handle table data returned from KDB+ queries, converting
 * them into C++ structures for further manipulation.
 */
class TableProcessor {
public:
    /**
     * @brief Processes a KDB+ table result.
     *
     * Converts the table into a KDBResult, handling cases where the result
     * contains a single row, multiple rows, or is empty.
     *
     * @param result Pointer to the KDB+ table result (K structure).
     * @return KDBResult The processed table result.
     */
    static KDBResult process_table_result(K result) {
        // Validate that the result is a table.
        if (!result || result->t != XT) {
            throw std::runtime_error("Invalid table result");
        }

        K dict = result->k;                   // Dictionary containing table data.
        K colvalues = kK(dict)[1];            // Column values.
        J num_rows = kK(colvalues)[0]->n;     // Number of rows.
        J num_cols = colvalues->n;            // Number of columns.

        // Handle different scenarios based on the number of rows.
        if (num_rows == 0) return KDBResult(KDBTable{});  // Empty table.
        if (num_rows == 1) return process_single_row(colvalues, num_cols);  // Single row.
        return process_multiple_rows(colvalues, num_rows, num_cols);        // Multiple rows.
    }

private:
    /**
     * @brief Processes a single-row table result.
     *
     * Extracts the data from each column and constructs a KDBRow.
     *
     * @param colvalues Pointer to the column values.
     * @param num_cols Number of columns.
     * @return KDBResult The processed single-row result.
     */
    static KDBResult process_single_row(K colvalues, J num_cols) {
        KDBRow row;
        for (J col = 0; col < num_cols; ++col) {
            K coldata = kK(colvalues)[col];  // Access column data.
            row.push_back(KDBValueConverter::convert_k_to_value(coldata, 0));  // Convert first element.
        }
        return KDBResult(std::move(row));
    }

    /**
     * @brief Processes a multiple-row table result.
     *
     * Iterates over each row and column, converting the data into a KDBTable.
     *
     * @param colvalues Pointer to the column values.
     * @param num_rows Number of rows.
     * @param num_cols Number of columns.
     * @return KDBResult The processed multiple-row result.
     */
    static KDBResult process_multiple_rows(K colvalues, J num_rows, J num_cols) {
        KDBTable table;
        for (J row = 0; row < num_rows; ++row) {
            KDBRow current_row;
            for (J col = 0; col < num_cols; ++col) {
                K coldata = kK(colvalues)[col];  // Access column data.
                current_row.push_back(KDBValueConverter::convert_k_to_value(coldata, row));  // Convert element.
            }
            table.push_back(std::move(current_row));  // Add row to table.
        }
        return KDBResult(std::move(table));
    }
};

/**
 * @class MetadataManager
 * @brief Manages retrieval and processing of table metadata from KDB+.
 *
 * Provides methods to query the metadata of a table, such as column names and types,
 * and to build a representation suitable for query building and result processing.
 */
class MetadataManager {
public:
    /**
     * @brief Retrieves metadata for a given table.
     *
     * Executes a 'meta' query on the specified table and extracts column names and types.
     *
     * @param table_name Name of the table.
     * @param internal_use Flag indicating if the metadata is for internal use.
     * @return std::vector<ColumnMeta> Vector of column metadata.
     */
    static std::vector<ColumnMeta> get_metadata(const std::string& table_name, const bool internal_use) {
        // Construct and execute the 'meta' query.
        std::string query = "select c, t from meta `" + table_name;
        auto query_result = inline_query(query);
        K result = query_result.get_result();

        // Validate the result.
        if (!validate_meta_result(result)) {
            return {};
        }

        K dict = result->k;                   // Dictionary containing metadata.
        K keys = kK(dict)[0];                 // Column names.
        K values = kK(dict)[1];               // Column types.

        // Extract and return metadata.
        return extract_metadata(keys, values, internal_use);
    }

private:
    /**
     * @brief Validates the result of the 'meta' query.
     *
     * Checks that the result is a table and does not contain an error.
     *
     * @param result Pointer to the KDB+ result.
     * @return bool True if valid, false otherwise.
     */
    static bool validate_meta_result(K result) {
        if (!result || result->t == -128 || result->t != XT) {
            std::cerr << "Failed to execute 'meta' query on table." << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Extracts metadata from the 'meta' query result.
     *
     * Builds a vector of ColumnMeta objects containing column names and type codes.
     *
     * @param keys Pointer to the keys from the dictionary (column names).
     * @param values Pointer to the values from the dictionary (column attributes).
     * @param internal_use Flag indicating if the metadata is for internal use.
     * @return std::vector<ColumnMeta> Vector of column metadata.
     */
    static std::vector<ColumnMeta> extract_metadata(K keys, K values, bool internal_use) {
        // Ensure keys and values are in the expected format.
        if (keys->t != KS || values->t != 0) {
            std::cerr << "Unexpected structure in meta result." << std::endl;
            return {};
        }

        // Map column names to their indices.
        std::unordered_map<std::string, int> col_index_map;
        for (J i = 0; i < keys->n; ++i) {
            col_index_map[std::string(kS(keys)[i])] = i;
        }

        // Check for required columns 'c' (column names) and 't' (types).
        if (col_index_map.find("c") == col_index_map.end() ||
            col_index_map.find("t") == col_index_map.end()) {
            return {};
        }

        // Build and return the metadata.
        return build_metadata(kK(values)[col_index_map["c"]],
                              kK(values)[col_index_map["t"]],
                              internal_use);
    }

    /**
     * @brief Builds metadata from column names and type symbols.
     *
     * Maps type symbols to type codes using the type map.
     *
     * @param c_col Pointer to the column names.
     * @param t_col Pointer to the type symbols.
     * @param internal_use Flag indicating if the metadata is for internal use.
     * @return std::vector<ColumnMeta> Vector of column metadata.
     */
    static std::vector<ColumnMeta> build_metadata(K c_col, K t_col, bool internal_use) {
        // Ensure columns are in the expected format.
        if (c_col->t != KS || t_col->t != KC) {
            std::cerr << "'c' or 't' column is not in expected format." << std::endl;
            return {};
        }

        const auto& type_map = getTypeMap();  // Get the mapping of type symbols to codes.
        std::vector<ColumnMeta> metadata;

        // Iterate over columns to build metadata.
        for (J i = 0; i < c_col->n; ++i) {
            ColumnMeta meta;
            meta.name = std::string(kS(c_col)[i]);  // Column name.
            std::string type_symbol(1, kC(t_col)[i]);  // Type symbol.

            // Map type symbol to type code.
            auto it = type_map.find(type_symbol);
            if (it != type_map.end()) {
                meta.type_code = it->second.first;
                if (!internal_use) {
                    std::cout << "Column: " << meta.name
                              << ", Type: " << it->second.second
                              << ", Type Code: " << meta.type_code << std::endl;
                }
            } else {
                std::cerr << "Unknown type symbol '" << type_symbol
                          << "' for column '" << meta.name << "'" << std::endl;
                meta.type_code = 0;
            }
            metadata.push_back(meta);
        }

        return metadata;
    }
};

/**
 * @class QueryBuilder
 * @brief Constructs queries for iloc and loc functions.
 *
 * Provides static methods to build KDB+ query strings based on row/column indices
 * or conditional expressions.
 */
class QueryBuilder {
public:
    /**
     * @struct FormattedValue
     * @brief Represents a value formatted for inclusion in a KDB+ query.
     *
     * Contains the formatted value string and a flag indicating if it's a char list.
     */
    struct FormattedValue {
        std::string value;   ///< The formatted value.
        bool is_charlist;    ///< Flag indicating if the value is a char list.
    };

    /**
     * @brief Builds a query string for iloc (index-based selection).
     *
     * Constructs a KDB+ query to select specific rows and columns by index.
     *
     * @param table_name Name of the table.
     * @param rows Vector of row indices.
     * @param cols Vector of column indices.
     * @return std::string The constructed query string.
     */
    static std::string build_iloc_query(const std::string& table_name,
                                        const std::vector<int>& rows,
                                        const std::vector<int>& cols) {
        // Format row indices.
        std::string row_indices_str = format_indices(rows, "til count " + table_name);
        // Format column indices.
        std::string column_indices_str = format_indices(cols, "til count cols " + table_name);

        // Construct the query using functional application.
        return "(0!" + table_name + ")[" + row_indices_str +
               ";(cols " + table_name + ")[" + column_indices_str + "]]";
    }

    /**
     * @brief Builds a query string for loc (condition-based selection).
     *
     * Constructs a KDB+ query to select rows based on conditional expressions.
     *
     * @param table_name Name of the table.
     * @param conditions Vector of condition strings.
     * @param metadata Vector of column metadata.
     * @return std::string The constructed query string.
     */
    static std::string build_loc_query(const std::string& table_name,
                                    const std::vector<std::string>& conditions,
                                    const std::vector<ColumnMeta>& metadata) {
        std::string query_result = table_name;

        static const std::unordered_map<std::string, std::string> valid_ops = {
            {">", ">"}, {"<", "<"}, {">=", ">="}, {"<=", "<="},
            {"==", "="}, {"=", "="}, {"!=", "<>"}, {"like", "like"},
            {"~", "~"}
        };

        // Updated regex to handle expressions on both sides
        // Now allows parentheses, arithmetic operators, and column names on both sides
        // Updated regex to be more flexible with whitespace
        std::regex condition_regex(R"(^\s*((?:[a-zA-Z][a-zA-Z0-9_]*(?:\([^)]*\))?|-?\d*\.?\d+|\(\s*[\w\s+\-*/()]+\s*\))(?:\s*[\+\-*/]\s*(?:[a-zA-Z][a-zA-Z0-9_]*(?:\([^)]*\))?|-?\d*\.?\d+|\(\s*[\w\s+\-*/()]+\s*\)))*)\s*([><=!~]{1,2}|like)\s*((?:[a-zA-Z][a-zA-Z0-9_]*(?:\([^)]*\))?|-?\d*\.?\d+|\(\s*[\w\s+\-*/()]+\s*\))(?:\s*[\+\-*/]\s*(?:[a-zA-Z][a-zA-Z0-9_]*(?:\([^)]*\))?|-?\d*\.?\d+|\(\s*[\w\s+\-*/()]+\s*\)))*)\s*$)");

        // Process each condition
        for (const auto& condition : conditions) {
            std::smatch matches;
            if (!std::regex_match(condition, matches, condition_regex)) {
                throw std::invalid_argument("Invalid condition format: " + condition);
            }

            query_result = process_condition(matches, metadata, valid_ops, query_result);
        }
        return query_result;
    }


private:
    /**
     * @brief Formats indices for inclusion in a query.
     *
     * Constructs a string representation of indices, or uses a default value if empty.
     *
     * @param indices Vector of indices.
     * @param default_value Default value if indices are empty.
     * @return std::string The formatted indices string.
     */
    
    static std::string format_indices(const std::vector<int>& indices, const std::string& default_value) {
        if (indices.empty()) return default_value;

        // Convert indices to a semicolon-separated string.
        return "(" + std::accumulate(
            std::next(indices.begin()),
            indices.end(),
            std::to_string(indices[0]),
            [](std::string a, int b) { return a + ";" + std::to_string(b); }
        ) + ")";
    }
    /**
     * @brief Checks if a string is a simple identifier (alphanumerics and underscores only).
     *
     * @param str The input string.
     * @return true If the string is a simple identifier.
     * @return false Otherwise.
     */
    static bool is_simple_identifier(const std::string& str) {
        return std::all_of(str.begin(), str.end(), [](char c) {
            return std::isalnum(c) || c == '_';
        });
    }
    static bool needs_evaluation(const std::string& expr) {
        // Check if expression contains arithmetic operators or parentheses
        return expr.find_first_of("+-*/()") != std::string::npos;
    }

    static std::string evaluate_expression(const std::string& expr, const std::vector<ColumnMeta>& metadata) {
        if (!needs_evaluation(expr)) {
            return expr;  // Return as is if it's a simple column or value
        }

        try {
            // Prepare the expression for KDB+ evaluation
            std::string kdb_expr = expr;
            
            // If the expression contains column names, we need to keep it as is
            // KDB+ will evaluate it in the context of the table
            return "(" + kdb_expr + ")";
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to evaluate expression: " + expr + " - " + e.what());
        }
    }

    static std::string process_condition(const std::smatch& matches,
                                     const std::vector<ColumnMeta>& metadata,
                                     const std::unordered_map<std::string, std::string>& valid_ops,
                                     const std::string& current_query) {
        std::string lhs = matches[1].str();    // Left hand expression
        std::string op = matches[2].str();     // Operator
        std::string rhs = matches[3].str();    // Right hand expression

        // Validate operator
        if (valid_ops.find(op) == valid_ops.end()) {
            throw std::invalid_argument("Invalid operator: " + op);
        }

        // Evaluate both sides if needed
        // Evaluate left hand side
        std::string evaluated_lhs = evaluate_expression(lhs, metadata);
        
        // For the right hand side, check if we're dealing with a symbol column
        std::string evaluated_rhs;
        
        // Find metadata for the column
        auto col_meta_iter = std::find_if(metadata.begin(), metadata.end(),
            [&](const ColumnMeta& meta) { return meta.name == lhs; });
            
        if (col_meta_iter != metadata.end() && col_meta_iter->type_code == KS && !needs_evaluation(rhs)) {
            // If it's a symbol column and RHS is a simple value, add backtick
            evaluated_rhs = "`" + rhs;
        } else {
            // Otherwise evaluate normally
            evaluated_rhs = evaluate_expression(rhs, metadata);
        }

        // Construct the updated query
        std::ostringstream query;
        query << "(0!select from (" << current_query
              << ") where " << evaluated_lhs << " "
              << valid_ops.at(op) << " " << evaluated_rhs << ")";

        return query.str();
    }



    /**
     * @brief Escapes a string for inclusion in a KDB+ query.
     *
     * Handles special characters such as quotes and backslashes.
     *
     * @param str The input string.
     * @return std::string The escaped string.
     */
    static std::string escape_string(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.length());
        for (char c : str) {
            if (c == '"' || c == '\\') {
                escaped += '\\';
            }
            escaped += c;
        }
        return escaped;
    }

    /**
     * @brief Formats a value for inclusion in a KDB+ query based on type.
     *
     * Handles different data types and appends appropriate type suffixes.
     *
     * @param value The value to format.
     * @param type_code The KDB+ type code.
     * @return FormattedValue The formatted value and a flag indicating if it's a char list.
     */
    static FormattedValue format_value_for_query(const std::string& value, int type_code) {
        std::cout << "Formatting value '" << value << "' for type code: " << type_code << std::endl;

        switch (type_code) {
            case KC:  // Character
            case -KC: // Char atom
            case 'C': // Meta might return this
            {
                // Handle as a char list (string).
                std::string formatted = "\"" + escape_string(value) + "\"";
                std::cout << "Handling as char list, formatted: " << formatted << std::endl;
                return {formatted, true};
            }
            case KS:  // Symbol
                return {"`" + value, false};
            case KB:  // Boolean
                return {value == "true" || value == "1" ? "1b" : "0b", false};
            // Date and time types with suffixes.
            case KD: return {value + "D", false};  // Date
            case KZ: return {value + "Z", false};  // DateTime
            case KT: return {value + "T", false};  // Time
            case KU: return {value + "u", false};  // Minute
            case KV: return {value + "v", false};  // Second
            case KM: return {value + "m", false};  // Month
            case KN: return {value + "N", false};  // Timespan
            // Numeric types.
            case KG:  // Byte
            case KH:  // Short
            case KI:  // Int
            case KJ:  // Long
            case KE:  // Real
            case KF:  // Float
                // Validate numeric format.
                if (!std::regex_match(value, std::regex(R"(-?\d*\.?\d+)"))) {
                    throw std::invalid_argument("Invalid numeric format");
                }
                return {value, false};
            default:
                std::cout << "Unknown type code: " << type_code << std::endl;
                return {value, false};
        }
    }
};

/**
 * @class ResultProcessor
 * @brief Processes results from iloc queries.
 *
 * Converts KDB+ query results into KDBResult objects, handling different result types.
 */
class ResultProcessor {
public:
    /**
     * @brief Processes the result of an iloc query.
     *
     * Handles scalar values, general lists, and vectors, converting them appropriately.
     *
     * @param result Pointer to the KDB+ result.
     * @return KDBResult The processed result.
     */
    static KDBResult process_iloc_result(K result) {
        // Check for null result.
        if (!result) {
            throw std::runtime_error("Query returned null result");
        }

        if (result->t < 0) {
            // Single scalar value.
            return KDBResult(KDBValueConverter::convert_k_to_value(result));
        } else if (result->t == 0) {
            // General list.
            return process_general_list(result);
        } else if (result->t > 0) {
            // Vector result (single column).
            return process_vector(result);
        }

        throw std::runtime_error("Unexpected result type");
    }

private:
    /**
     * @brief Processes a general list result.
     *
     * Handles single-row and multiple-row results, converting them into KDBRow or KDBTable.
     *
     * @param result Pointer to the KDB+ general list.
     * @return KDBResult The processed result.
     */
    static KDBResult process_general_list(K result) {
        if (result->n == 0) {
            // Empty result.
            return KDBResult(KDBRow{});
        }

        if (kK(result)[0]->t == 0) {
            // Multiple rows.
            KDBTable table;
            for (J i = 0; i < result->n; ++i) {
                K row_data = kK(result)[i];
                KDBRow current_row;
                for (J j = 0; j < row_data->n; ++j) {
                    current_row.push_back(KDBValueConverter::convert_k_to_value(kK(row_data)[j]));
                }
                table.push_back(std::move(current_row));
            }
            return KDBResult(std::move(table));
        } else {
            // Single row with multiple columns.
            KDBRow row;
            for (J i = 0; i < result->n; ++i) {
                row.push_back(KDBValueConverter::convert_k_to_value(kK(result)[i]));
            }
            return KDBResult(std::move(row));
        }
    }

    /**
     * @brief Processes a vector result.
     *
     * Converts the vector into a KDBRow.
     *
     * @param result Pointer to the KDB+ vector.
     * @return KDBResult The processed result.
     */
    static KDBResult process_vector(K result) {
        KDBRow row;
        for (J i = 0; i < result->n; ++i) {
            row.push_back(KDBValueConverter::convert_k_to_value(result, i));
        }
        return KDBResult(std::move(row));
    }
};

} // namespace kdb_utils

// Main interface functions

/**
 * @brief Selects rows and columns from a table using index-based selection.
 *
 * Provides functionality similar to DataFrame.iloc in pandas, allowing selection
 * by integer indices.
 *
 * @param table_name Name of the table in KDB+.
 * @param rows Vector of row indices to select.
 * @param cols Vector of column indices to select.
 * @return KDBResult The selected data.
 */
KDBResult iloc(const std::string& table_name,
               const std::vector<int>& rows,
               const std::vector<int>& cols) {
    try {
        // Retrieve metadata for bounds checking.
        auto metadata = kdb_utils::MetadataManager::get_metadata(table_name, true);
        if (metadata.empty()) {
            throw std::runtime_error("Invalid table name or empty table");
        }

        // Get row count for the table.
        auto count_result = inline_query("count " + table_name);
        K count = count_result.get_result();
        if (!count || count->t != -KJ) {
            throw std::runtime_error("Failed to get table row count");
        }
        J row_count = count->j;

        // Validate row indices.
        for (J row : rows) {
            if (row < 0 || row >= row_count) {
                throw std::out_of_range("Row index out of bounds: " + std::to_string(row));
            }
        }
        // Validate column indices.
        for (J col : cols) {
            if (col < 0 || col >= static_cast<J>(metadata.size())) {
                throw std::out_of_range("Column index out of bounds: " + std::to_string(col));
            }
        }

        // Build and execute the query.
        std::string query = kdb_utils::QueryBuilder::build_iloc_query(table_name, rows, cols);
        //std::cout << "Executing query: " << query << std::endl;

        auto query_result = inline_query(query);
        return kdb_utils::ResultProcessor::process_iloc_result(query_result.get_result());

    } catch (const std::exception& e) {
        std::cerr << "Error in iloc: " << e.what() << std::endl;
        throw;
    }
}

/**
 * @brief Selects rows from a table using condition-based selection.
 *
 * Provides functionality similar to DataFrame.loc in pandas, allowing selection
 * based on conditional expressions.
 *
 * @param table_name Name of the table in KDB+.
 * @param conditions Comma-separated string of conditions.
 * @return KDBResult The selected data.
 */
KDBResult loc(const std::string& table_name, const std::string& conditions) {
    try {
        // Retrieve and validate metadata.
        auto metadata = kdb_utils::MetadataManager::get_metadata(table_name, true);
        if (metadata.empty()) {
            throw std::runtime_error("Invalid table name or empty table");
        }

        // Split conditions into a vector.
        std::vector<std::string> condition_list;
        std::istringstream stream(conditions);
        std::string condition;
        while (std::getline(stream, condition, ',')) {
            // Trim whitespace.
            condition.erase(0, condition.find_first_not_of(" \t\n\r"));
            condition.erase(condition.find_last_not_of(" \t\n\r") + 1);
            if (!condition.empty()) {
                condition_list.push_back(condition);
            }
        }

        // Build and execute the query.
        std::string query = kdb_utils::QueryBuilder::build_loc_query(table_name, condition_list, metadata);
        //std::cout << "Executing query: " << query << std::endl;

        auto query_result = inline_query(query);
        K k_result = query_result.get_result();

        if (!k_result) {
            throw std::runtime_error("Query returned null result");
        }

        return kdb_utils::TableProcessor::process_table_result(k_result);

    } catch (const std::exception& e) {
        std::cerr << "Error in loc: " << e.what() << std::endl;
        throw;
    }
}

	
