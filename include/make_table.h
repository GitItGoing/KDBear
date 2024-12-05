#ifndef MAKE_TABLE_H
#define MAKE_TABLE_H

#include <vector>
#include <string>
#include <variant>
#include "k.h"
#include "connections.h"  

/**
 * @brief Defines the possible data types that can be stored in a KDB+ table.
 *
 * The `KDBType` variant can hold one of the following types:
 * - `std::monostate`: Represents a null value.
 * - `bool`: Boolean values (`true` or `false`).
 * - `int`: Integer values.
 * - `double`: Floating-point values.
 * - `std::string`: String values.
 */
using KDBType = std::variant<std::monostate, bool, int, double, std::string>;

/**
 * @brief Creates a KDB+ table with the specified name, columns, and data.
 *
 * This function constructs a q-sql command to create a table in KDB+ with the given
 * `table_name`, `column_names`, and `data`. It handles both single-row and
 * multi-row data, ensuring proper formatting and escaping of values. The command
 * is then executed using the `inline_query` function.
 *
 * @param table_name The name of the table to be created in KDB+.
 * @param column_names A vector of strings representing the names of the columns.
 * @param data A two-dimensional vector containing the data for each column.
 *             Each inner vector corresponds to a row, and each element within
 *             the inner vector corresponds to a column value.
 * @return bool Returns `true` if the table is created successfully; otherwise, returns `false`.
 */
bool make_table(const std::string& table_name,
               const std::vector<std::string>& column_names,
               const std::vector<std::vector<KDBType>>& data);

#endif // MAKE_TABLE_H
