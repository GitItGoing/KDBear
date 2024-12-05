#include "table_structure.h"
#include "inline_query.h"
#include <iomanip>
#include <iostream>

/**
 * @brief Retrieves a K table object from a table name using inline query
 * @param table_name Name of the table to retrieve
 * @return K Pointer to K object containing the table, or nullptr if query fails
 */
K get_table_from_name(const std::string& table_name) {
    auto result = inline_query(table_name);
    return result.get_result();
}

/**
 * @brief Resolves table input that can be either a K object or table name
 * @param table_input Variant containing either K table pointer or table name string
 * @param needs_cleanup Reference to bool indicating if returned K object needs cleanup
 * @return K Pointer to resolved K table object, or nullptr if resolution fails
 * @note If table_input contains a K object, it is reference counted. If it contains
 *       a string, a new K object is created that needs cleanup
 */
K resolve_table_input(const std::variant<K, std::string>& table_input, bool& needs_cleanup) {
    needs_cleanup = false;
    
    if (std::holds_alternative<K>(table_input)) {
        K table = std::get<K>(table_input);
        return table ? r1(table) : nullptr;
    } else {
        needs_cleanup = true;
        return get_table_from_name(std::get<std::string>(table_input));
    }
}
/**
 * @brief Gets the dimensions (rows × columns) of a table
 * @param table_input Variant containing either K table pointer or table name string
 * @return tuple<int,int> Pair of (row_count, column_count), returns (-1,-1) on error
 * @note For string input, executes a query to get dimensions
 *       For K table input, calculates dimensions directly from table structure
 * @note Handles memory cleanup appropriately for both input types
 */
std::tuple<int, int> shape(const std::variant<K, std::string>& table_input) {
    bool needs_cleanup = false;
    K table_to_use = resolve_table_input(table_input, needs_cleanup);
    
    if (!table_to_use) {
        std::cerr << "Error: Could not resolve table input." << std::endl;
        return std::make_tuple(-1, -1);
    }

    if (table_to_use->t != XT) {
        std::cerr << "Error: Not a valid kdb+ table." << std::endl;
        if (needs_cleanup) r0(table_to_use);
        return std::make_tuple(-1, -1);
    }

    // Execute query for string input
    if (std::holds_alternative<std::string>(table_input)) {
        std::string table_name = std::get<std::string>(table_input);
        std::string query = "(count " + table_name + ";count cols " + table_name + ")";
        auto result = inline_query(query);
        
        K result_k = result.get_result();
        if (!result_k || result_k->t != 7) {  // Check for long vector type
            std::cerr << "Error: Expected long vector result" << std::endl;
            if (needs_cleanup) r0(table_to_use);
            return std::make_tuple(-1, -1);
        }

        J* data = kJ(result_k);
        if (!data) {
            std::cerr << "Error: Could not access result data" << std::endl;
            if (needs_cleanup) r0(table_to_use);
            return std::make_tuple(-1, -1);
        }

        int row_count = static_cast<int>(data[0]);
        int col_count = static_cast<int>(data[1]);

        if (needs_cleanup) r0(table_to_use);
        r0(result_k);  // Clean up the result
        return std::make_tuple(row_count, col_count);
    } else {
        // For K table input, calculate shape directly from the table structure
        K dict = table_to_use->k;  // Get dictionary from table
        if (!dict) {
            std::cerr << "Error: Could not access table dictionary" << std::endl;
            if (needs_cleanup) r0(table_to_use);
            return std::make_tuple(-1, -1);
        }

        K values = kK(dict)[1];    // Get values part of dictionary
        if (!values) {
            std::cerr << "Error: Could not access table values" << std::endl;
            if (needs_cleanup) r0(table_to_use);
            return std::make_tuple(-1, -1);
        }

        // For row count, use the length of any column
        int row_count = values->n;  // Number of rows
        
        // For column count, use the length of the values list itself
        K cols = kK(dict)[0];  // Get column names
        int col_count = cols->n;    // Number of columns
        
        if (needs_cleanup) r0(table_to_use);
        return std::make_tuple(row_count, col_count);
    }
}
