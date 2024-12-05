#include "joins.h"
#include <iostream>
#include <iomanip>
#include <sstream>  
#include "inline_query.h"

/**
 * @namespace joins
 * @brief Contains functions related to performing various types of joins on kdb+ tables.
 */
namespace joins {

/**
 * @namespace detail
 * @brief Provides internal helper functions for join operations.
 */
namespace detail {

/**
 * @brief Prepares unkeyed versions of the input tables for joining.
 *
 * This function creates unkeyed copies of the provided tables by prefixing them with
 * "_unkeyed". It constructs and executes a kdb+ query to perform this operation.
 *
 * @param table1 The name of the first table to prepare.
 * @param table2 The name of the second table to prepare.
 * @param t1_unkeyed Reference to a string that will hold the name of the unkeyed first table.
 * @param t2_unkeyed Reference to a string that will hold the name of the unkeyed second table.
 * @return true If the tables were prepared successfully.
 * @return false If the preparation query failed.
 *
 * Note: The function appends "_unkeyed" to table names. Ensure original table
 * names don't already end with this suffix to avoid conflicts.
 */
bool prepare_tables(const std::string& table1,
                   const std::string& table2,
                   std::string& t1_unkeyed,
                   std::string& t2_unkeyed) {
    t1_unkeyed = table1 + "_unkeyed";
    t2_unkeyed = table2 + "_unkeyed";

    // Construct the preparation query to create unkeyed tables
    std::string prep_query = t1_unkeyed + ": 0!(" + table1 + ");";
    prep_query += t2_unkeyed + ": 0!(" + table2 + ");";

    //std::cout << "\nPreparing tables with query: " << prep_query << std::endl;
    
    // Execute the preparation query
    auto prep_result = inline_query(prep_query);
    return bool(prep_result);  // Convert the result to a boolean indicating success
}

/**
 * @brief Cleans up temporary unkeyed tables after join operations.
 *
 * This function deletes the temporary unkeyed tables from the kdb+ environment to free up resources.
 *
 * @param t1_unkeyed The name of the first unkeyed table to delete.
 * @param t2_unkeyed The name of the second unkeyed table to delete.
 */
void cleanup_tables(const std::string& t1_unkeyed, const std::string& t2_unkeyed) {
    // Construct the cleanup query to delete unkeyed tables
    std::string cleanup_query = "delete " + t1_unkeyed + " from `.; "
                              "delete " + t2_unkeyed + " from `.;";
    //std::cout << "\nCleaning up with query: " << cleanup_query << std::endl;

    // Execute the cleanup query
    inline_query(cleanup_query);
}

/**
 * @brief Builds the `by` clause for join queries based on specified join columns.
 *
 * This function concatenates the join column names into a single string formatted for kdb+ queries.
 * IMPORTANT Note: Backticks for column names are handled by the calling function.
 *      This means that a user does not do: ticker = `AAPL
 *      But rather, ticker = AAPL
 *
 * @param join_columns A vector of column names to be used for joining.
 * @return std::string The constructed `by` clause for the join query.
 */
std::string build_join_by(const std::vector<std::string>& join_columns) {
    if (join_columns.empty()) return "";
    std::string join_by = " by ";
    for (size_t i = 0; i < join_columns.size(); ++i) {
        if (i > 0) join_by += ",";
        join_by += join_columns[i];
    }
    return join_by;
}

/**
 * @brief Executes a join query and handles the cleanup of temporary tables.
 *
 * This function executes the provided kdb+ join query, retrieves the result, and ensures that
 * temporary unkeyed tables are deleted regardless of the join's success.
 *
 * @param query The kdb+ join query to execute.
 * @param result_name The name under which the joined table will be stored.
 * @param t1_unkeyed The name of the first temporary unkeyed table.
 * @param t2_unkeyed The name of the second temporary unkeyed table.
 * @return K The kdb+ object representing the joined table, or nullptr if the join failed.
 */
K execute_join(const std::string& query,
              const std::string& result_name,
              const std::string& t1_unkeyed,
              const std::string& t2_unkeyed) {
    //std::cout << "\nExecuting KDB+ Query: " << query << std::endl;
    
    // Execute the join operation
    auto exec_result = inline_query(query);
    if (!bool(exec_result)) {
        // If the join fails, perform cleanup and return nullptr
        cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }

    // Retrieve the joined table by its result name
    auto result = inline_query(result_name);
    cleanup_tables(t1_unkeyed, t2_unkeyed);

    if (!bool(result)) {
        return nullptr;
    }

    //std::cout << "Saved joined table as: '" << result_name << "'" << std::endl;
    return result.get_result();
}

} // namespace detail

/**
 * @brief Performs an inner join between two kdb+ tables.
 *
 * This function joins two tables based on specified join columns. If no join columns are provided,
 * it performs a natural inner join. The resulting table is stored under the provided result name.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param join_columns A vector of column names to join on. If empty, a natural join is performed.
 * @return K The kdb+ object representing the joined table, or (K)0 if the join failed.
 */
K inner_join(const std::string& table1,
             const std::string& table2,
             const std::string& result_name,
             const std::vector<std::string>& join_columns) {
    std::string t1_unkeyed, t2_unkeyed;
    
    if (!detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) return (K)0;

    std::string query;
    if (!join_columns.empty()) {
        std::string key_cols = join_columns[0];
        for (size_t i = 1; i < join_columns.size(); ++i) {
            key_cols += "`" + join_columns[i];
        }
        query = result_name + ": " + t1_unkeyed + " ij `" + key_cols + " xkey " + t2_unkeyed;
    } else {
        query = result_name + ": " + t1_unkeyed + " ij (enlist first cols[" + t1_unkeyed +
                "] inter cols[" + t2_unkeyed + "]) xkey " + t2_unkeyed;
    }
    return detail::execute_join(query, result_name, t1_unkeyed, t2_unkeyed);
}

/**
 * @brief Performs an as-of join between two kdb+ tables.
 *
 * This function joins two tables based on specified join columns and aligns rows based on
 * the nearest preceding or exact time in the specified time columns.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param time_column_left The name of the time column in the first table.
 * @param time_column_right The name of the time column in the second table.
 * @param join_columns A vector of column names to join on, excluding the time columns.
 * @return K The kdb+ object representing the joined table, or nullptr if the join failed.
 */
K asof_join(const std::string& table1,
            const std::string& table2,
            const std::string& result_name,
            const std::string& time_column_left,
            const std::string& time_column_right,
            const std::vector<std::string>& join_columns) {
    std::string t1_unkeyed, t2_unkeyed;
    
    // Prepare unkeyed versions of the input tables
    if (!detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) return nullptr;

    // Build the join columns string excluding the time columns
    std::string join_by = join_columns.empty() ? "" : join_columns[0];
    for (size_t i = 1; i < join_columns.size(); ++i) {
        join_by += "`" + join_columns[i];
    }

    // Rename the right table's time column to avoid name collision
    std::string rename_query = "update " + time_column_right + "2:" + time_column_right + " from " + t2_unkeyed;
    auto rename_result = inline_query(t2_unkeyed + "_adj: " + rename_query);
    if (!bool(rename_result)) {
        std::cerr << "Failed to rename time column in right table for asof join." << std::endl;
        detail::cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }

    // Build the as-of join query
    std::string aj_query;
    if (!join_by.empty()) {
        aj_query = result_name + ": aj[`" + join_by + "`" + time_column_left + ";" +
                   t1_unkeyed + ";" + t2_unkeyed + "_adj]";
    } else {
        aj_query = result_name + ": aj[`" + time_column_left + ";" +
                   t1_unkeyed + ";" + t2_unkeyed + "_adj]";
    }

    // Execute the as-of join
    K result = detail::execute_join(aj_query, result_name, t1_unkeyed, t2_unkeyed + "_adj");

    // Clean up the adjusted right table
    inline_query("delete " + t2_unkeyed + "_adj from `.");

    return result;
}

/**
 * @brief Performs a left join between two kdb+ tables.
 *
 * This function joins two tables, retaining all rows from the first table and matching rows from the second table.
 * If no join columns are specified, a natural left join is performed.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param join_columns A vector of column names to join on. If empty, a natural join is performed.
 * @return K The kdb+ object representing the joined table, or (K)0 if the join failed.
 */
K left_join(const std::string& table1,
            const std::string& table2,
            const std::string& result_name,
            const std::vector<std::string>& join_columns) {
    std::string t1_unkeyed, t2_unkeyed;
    
    if (!detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) return (K)0;

    std::string query;
    if (!join_columns.empty()) {
        std::string key_cols = join_columns[0];
        for (size_t i = 1; i < join_columns.size(); ++i) {
            key_cols += "`" + join_columns[i];
        }
        query = result_name + ": " + t1_unkeyed + " lj `" + key_cols + " xkey " + t2_unkeyed;
    } else {
        query = result_name + ": " + t1_unkeyed + " lj (enlist first cols[" + t1_unkeyed +
                "] inter cols[" + t2_unkeyed + "]) xkey " + t2_unkeyed;
    }
    return detail::execute_join(query, result_name, t1_unkeyed, t2_unkeyed);
}

/**
 * @brief Performs a right join between two kdb+ tables.
 *
 * This function joins two tables, retaining all rows from the second table and matching rows from the first table.
 * If no join columns are specified, a natural right join is performed.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param join_columns A vector of column names to join on. If empty, a natural join is performed.
 * @return K The kdb+ object representing the joined table, or (K)0 if the join failed.
 */
K right_join(const std::string& table1,
             const std::string& table2,
             const std::string& result_name,
             const std::vector<std::string>& join_columns) {
    std::string t1_unkeyed, t2_unkeyed;
    
    if (!detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) return (K)0;

    std::string query;
    if (!join_columns.empty()) {
        std::string key_cols = join_columns[0];
        for (size_t i = 1; i < join_columns.size(); ++i) {
            key_cols += "`" + join_columns[i];
        }
        query = result_name + ": " + t2_unkeyed + " lj `" + key_cols + " xkey " + t1_unkeyed;
    } else {
        query = result_name + ": " + t2_unkeyed + " lj (enlist first cols[" + t1_unkeyed +
                "] inter cols[" + t2_unkeyed + "]) xkey " + t1_unkeyed;
    }
    return detail::execute_join(query, result_name, t1_unkeyed, t2_unkeyed);
}

/**
 * @brief Performs a window join between two kdb+ tables.
 *
 * This function joins two tables based on temporal proximity, allowing for a specified window size
 * within which rows from the second table are matched to rows in the first table.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param time_column_left The name of the time column in the first table.
 * @param time_column_right The name of the time column in the second table.
 * @param window_size_seconds The size of the window (in seconds) for matching rows.
 * @param join_columns A vector of column names to join on.
 * @return K The kdb+ object representing the joined table, or nullptr if the join failed.
 */
K window_join(const std::string& table1,
              const std::string& table2,
              const std::string& result_name,
              const std::string& time_column_left,
              const std::string& time_column_right,
              double window_size_seconds,
              const std::vector<std::string>& join_columns) {
    // Prepare unkeyed versions of the input tables
    std::string t1_unkeyed, t2_unkeyed;
    if (!joins::detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) {
        return nullptr;
    }

    // Ensure that join columns are specified for window joins
    if (join_columns.empty()) {
        std::cerr << "Window joins require join columns to be specified." << std::endl;
        joins::detail::cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }

    // Build key columns string including the time column
    std::string key_cols;
    for (size_t i = 0; i < join_columns.size(); ++i) {
        if (i > 0) key_cols += ",";
        key_cols += "`" + join_columns[i];
    }
    key_cols += ",`" + time_column_left;

    // Convert window size to q time literal format
    int total_seconds = static_cast<int>(window_size_seconds);
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    // Format the window interval as time literals
    std::ostringstream window_interval;
    window_interval << "(-00:"
                    << std::setfill('0') << std::setw(2) << minutes << ":"
                    << std::setfill('0') << std::setw(2) << seconds << ".000 "
                    << "00:"
                    << std::setfill('0') << std::setw(2) << minutes << ":"
                    << std::setfill('0') << std::setw(2) << seconds << ".000)";

    // Create per-row window intervals by adding the window size to the left table's time column
    std::string window_query = "window: " + window_interval.str() + " +\\: " +
                               t1_unkeyed + "`" + time_column_left;

    // Execute the window interval creation query
    auto window_result = inline_query(window_query);
    if (!bool(window_result)) {
        std::cerr << "Failed to create window intervals." << std::endl;
        joins::detail::cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }

    // Determine columns to include from the right table, excluding join and time columns
    std::string exclude_cols = "`" + time_column_right;
    for (const auto& col : join_columns) {
        exclude_cols += "`" + col;
    }

    // Retrieve the columns to include from the right table
    std::string include_cols_query = "(cols " + t2_unkeyed + ") except " + exclude_cols;
    auto include_cols_result = inline_query(include_cols_query);
    if (!bool(include_cols_result)) {
        std::cerr << "Failed to get columns from the right table." << std::endl;
        joins::detail::cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }
    K include_cols_k = include_cols_result.get_result();

    // Build the function-column pairs using 'last' to handle multiple matches within the window
    std::string func_col_pairs = "(" + t2_unkeyed;
    for (J i = 0; i < include_cols_k->n; ++i) {
        S col_name = kS(include_cols_k)[i];
        func_col_pairs += "; (last; `" + std::string(col_name) + ")";
    }
    func_col_pairs += ")";

    // Release the reference to include_cols_k to prevent memory leaks
    r0(include_cols_k);

    // Construct the window join query
    std::string query = result_name + ": wj[window; " + key_cols + "; " + t1_unkeyed + "; " +
                        func_col_pairs + "]";

    // Execute the window join
    K result = joins::detail::execute_join(query, result_name, t1_unkeyed, t2_unkeyed);
    if (!result) {
        std::cerr << "Window join execution failed." << std::endl;
        joins::detail::cleanup_tables(t1_unkeyed, t2_unkeyed);
        return nullptr;
    }

    // Clean up the window variable from the kdb+ environment
    inline_query("delete window from `.");

    return result;
}

/**
 * @brief Performs a union join between two kdb+ tables.
 *
 * This function combines two tables by appending the rows of the second table to the first,
 * effectively performing a union operation. The resulting table is stored under the provided result name.
 *
 * @param table1 The name of the first table to join.
 * @param table2 The name of the second table to join.
 * @param result_name The name under which the joined table will be stored.
 * @param join_columns A vector of column names to join on. (Note: Union join typically doesn't require join columns.)
 * @return K The kdb+ object representing the joined table, or (K)0 if the join failed.
 *
 * Note: Tables should have matching column structures. If columns differ,
 * null values will be used for missing columns in either table.
 */
K union_join(const std::string& table1,
             const std::string& table2,
             const std::string& result_name,
             const std::vector<std::string>& join_columns) {
    std::string t1_unkeyed, t2_unkeyed;
    
    // Prepare unkeyed versions of the input tables
    if (!detail::prepare_tables(table1, table2, t1_unkeyed, t2_unkeyed)) return (K)0;

    // Construct the union join query by appending the second table to the first
    std::string query = result_name + ": " + t1_unkeyed + " uj " + t2_unkeyed;

    // Execute the union join and handle cleanup
    return detail::execute_join(query, result_name, t1_unkeyed, t2_unkeyed);
}

} // namespace joins
