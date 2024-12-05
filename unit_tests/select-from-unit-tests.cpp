#include "select_from_table.h"
#include "inline_query.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

// Helper function for test results
void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Test failed: " + message);
    }
}

// Helper function to get numeric value from KDBValue
long long get_numeric_value(const KDBValue& val) {
    if (val.is_long()) return val.get_long();
    if (val.is_integer()) return val.get_integer();
    throw std::runtime_error("Value is neither integer nor long");
}

// Test iloc with unkeyed table
void test_iloc_unkeyed_table() {
    std::cout << "Testing iloc with unkeyed table (table1)..." << std::endl;

    // Create test table
    inline_query("table1:([] ticker:`GOOG`MSFT`AAPL;price:20 30 40;size:10 20 30)");

    // Single row, single column
    {
        std::vector<int> rows = {0};
        std::vector<int> cols = {0};
        auto result = iloc("table1", rows, cols);
        check(result.is_value(), "Expected single value for single row, single column");
        check(result.get_value().get_symbol() == "GOOG", "Value mismatch for single row, single column");
    }

    // Single row, multiple columns
    {
        std::vector<int> rows = {1};
        std::vector<int> cols = {0, 1};
        auto result = iloc("table1", rows, cols);
        check(result.is_row(), "Expected row for single row, multiple columns");
        const auto& row = result.get_row();
        check(row.size() == 2, "Row size mismatch for single row, multiple columns");
        
        check(row[0].is_symbol(), "First column should be a symbol");
        check(row[0].get_symbol() == "MSFT", "First column mismatch");
        
        std::cout << "Second column type code: " << static_cast<int>(row[1].get_type()) << std::endl;
        check(get_numeric_value(row[1]) == 30, "Second column numeric value mismatch");
    }

    // Multiple rows, single column
    {
        std::vector<int> rows = {0, 2};
        std::vector<int> cols = {1};
        auto result = iloc("table1", rows, cols);
        check(result.is_row(), "Expected row for multiple rows, single column");
        const auto& column = result.get_row();
        check(column.size() == 2, "Column size mismatch");
        
        check(get_numeric_value(column[0]) == 20, "First value mismatch");
        check(get_numeric_value(column[1]) == 40, "Second value mismatch");
    }

    // All rows, all columns
    {
        std::vector<int> rows = {0, 1, 2};
        std::vector<int> cols = {0, 1, 2};
        auto result = iloc("table1", rows, cols);
        check(result.is_table(), "Expected table for all rows, all columns");
        const auto& table = result.get_table();
        check(table.size() == 3, "Table row size mismatch");

        // Validate each row
        check(table[0][0].get_symbol() == "GOOG", "First row, first column mismatch");
        check(get_numeric_value(table[1][1]) == 30, "Second row, second column mismatch");
        check(get_numeric_value(table[2][2]) == 30, "Third row, third column mismatch");
    }

    // Empty rows
    {
        std::vector<int> rows;
        std::vector<int> cols = {1};
        auto result = iloc("table1", rows, cols);
        check(result.is_row(), "Expected row for empty row selection");
        const auto& column = result.get_row();
        check(column.size() == 3, "Column size mismatch for empty row selection");
        
        // Validate numeric values in the column
        for (size_t i = 0; i < column.size(); ++i) {
            check(column[i].is_long() || column[i].is_integer(),
                  "Column value at index " + std::to_string(i) + " is not numeric");
        }
    }

    // Out-of-bounds rows
    {
        std::vector<int> rows = {5};
        std::vector<int> cols = {0};
        try {
            iloc("table1", rows, cols);
            check(false, "Expected out-of-bounds exception");
        } catch (const std::out_of_range&) {
            // Expected exception
        }
    }
}

// Test iloc with keyed table
void test_iloc_keyed_table() {
    std::cout << "Testing iloc with keyed table (table2)..." << std::endl;

    // Create test table
    inline_query("table2:([ticker:`GOOG`AAPL] bid:19 39; ask:22 44; size:50 40)");

    // Single row
    {
        std::vector<int> rows = {0};
        std::vector<int> cols = {0, 1};
        auto result = iloc("table2", rows, cols);
        check(result.is_row(), "Expected row for single row");
        const auto& row = result.get_row();
        check(row[0].get_symbol() == "GOOG", "First column mismatch for single row");
        check(get_numeric_value(row[1]) == 19, "Second column mismatch for single row");
    }
}

// Test loc with conditions
void test_loc() {
    std::cout << "Testing loc function..." << std::endl;

    // Unkeyed table
    {
        inline_query("table1:([] ticker:`GOOG`MSFT`AAPL;price:20 30 40;size:10 20 30)");
        auto result = loc("table1", "ticker=GOOG");
        check(result.is_row(), "Expected row for loc with equality condition");
        const auto& row = result.get_row();
        check(row[0].get_symbol() == "GOOG", "Symbol mismatch in loc result");
        check(get_numeric_value(row[1]) == 20, "Price mismatch in loc result");
    }

    // Keyed table
    {
        inline_query("table2: ([ticker:`GOOG`AAPL] bid:19 39; ask:22 44; size:50 40; strCol:`info1`info2)");
        auto result = loc("table2", "ticker=AAPL");
        check(result.is_row(), "Expected row for loc with key condition");
        const auto& row = result.get_row();
        check(row[0].get_symbol() == "AAPL", "Symbol mismatch in keyed table loc result");
        check(get_numeric_value(row[1]) == 39, "Bid mismatch in keyed table loc result");
    }
}

int main() {
    try {
        // Initialize connection
        if (!KDBConnection::connect("localhost", 6000)) {
            throw std::runtime_error("Failed to connect to KDB+ server");
        }

        test_iloc_unkeyed_table();
        test_iloc_keyed_table();
        test_loc();

        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

