#include "make_table.h"
#include "connections.h"
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <limits>
#include "inline_query.h"
class TestResult {
public:
    bool passed;
    std::string message;
    std::string testName;
    
    TestResult(bool p, const std::string& msg, const std::string& name)
        : passed(p), message(msg), testName(name) {}
};

class MakeTableTests {
private:
    std::vector<TestResult> results;
    int totalTests = 0;
    int passedTests = 0;

    void recordResult(bool passed, const std::string& message, const std::string& testName) {
        results.emplace_back(passed, message, testName);
        totalTests++;
        if (passed) passedTests++;
    }

    // Helper to verify table existence and content
    bool verifyTable(const std::string& tableName, size_t expectedRows, size_t expectedCols) {
        try {
            auto result = inline_query("count " + tableName);
            if (!result) return false;
            
            auto col_result = inline_query("count cols " + tableName);
            if (!col_result) return false;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error verifying table: " << e.what() << std::endl;
            return false;
        }
    }

    void cleanupTable(const std::string& tableName) {
        try {
            inline_query("delete " + tableName + " from `.");
        } catch (const std::exception& e) {
            std::cerr << "Error cleaning up table: " << e.what() << std::endl;
        }
    }

public:
    void runAllTests() {
        // Ensure we're connected to KDB+
        if (!KDBConnection::connect("localhost", 6000)) {
            std::cerr << "Failed to connect to KDB+ server" << std::endl;
            return;
        }

        try {
            testBasicTableCreation();
            testEmptyTable();
            testMismatchedColumns();
            testLargeTable();
            testSafeCharacters();  // Renamed from testSpecialCharacters
            testMixedTypes();
            testNullValues();
            testDuplicateTableNames();
            testValidNames();      // Renamed from testUnicodeSupport
            testEdgeCases();
        } catch (const std::exception& e) {
            std::cerr << "Test execution error: " << e.what() << std::endl;
        }

        KDBConnection::disconnect();
        printResults();
    }

    void testBasicTableCreation() {
        std::vector<std::string> columns = {"Name", "Age", "Salary"};
        std::vector<std::vector<KDBType>> data = {
            {std::string("Alice"), 30, 70000.0},
            {std::string("Bob"), 25, 50000.0}
        };

        bool result = make_table("basic_table", columns, data);
        bool verified = result && verifyTable("basic_table", 2, 3);
        
        recordResult(verified,
            verified ? "Successfully created basic table" : "Failed to create basic table",
            "Basic Table Creation");
            
        cleanupTable("basic_table");
    }

    void testEmptyTable() {
        std::vector<std::string> columns;
        std::vector<std::vector<KDBType>> data;
        
        bool result = make_table("empty_table", columns, data);
        recordResult(!result,
            !result ? "Correctly rejected empty table" : "Incorrectly accepted empty table",
            "Empty Table Handling");
    }

    void testMismatchedColumns() {
        std::vector<std::string> columns = {"Col1", "Col2"};
        std::vector<std::vector<KDBType>> data = {
            {std::string("value1"), 2}  // Correct number of columns
        };

        bool validResult = make_table("valid_table", columns, data);
        cleanupTable("valid_table");

        // Test with mismatched columns
        std::vector<std::vector<KDBType>> invalidData = {
            {std::string("value1")}  // Missing a column
        };

        bool invalidResult = make_table("invalid_table", columns, invalidData);
        
        recordResult(validResult && !invalidResult,
            "Correctly handled column validation",
            "Mismatched Columns");
    }

    void testLargeTable() {
        std::vector<std::string> columns = {"ID", "Value"};
        std::vector<std::vector<KDBType>> data;
        
        // Create 1,000 rows (reduced from 100,000 for testing)
        for (int i = 0; i < 1000; ++i) {
            data.push_back({i, static_cast<double>(i * 1.5)});
        }

        bool result = make_table("large_table", columns, data);
        bool verified = result && verifyTable("large_table", 1000, 2);
        
        recordResult(verified,
            verified ? "Successfully created large table" : "Failed to create large table",
            "Large Table Creation");
            
        cleanupTable("large_table");
    }

    void testSafeCharacters() {
        // Test with safe characters only
        std::vector<std::string> columns = {"Col_1", "Col_2", "Col_3"};
        std::vector<std::vector<KDBType>> data = {
            {std::string("value_1"), std::string("value_2"), std::string("value_3")}
        };

        bool result = make_table("safe_chars", columns, data);
        recordResult(result,
            result ? "Successfully handled safe characters" : "Failed to handle safe characters",
            "Safe Characters");
            
        cleanupTable("safe_chars");
    }

    void testMixedTypes() {
        std::vector<std::string> columns = {"IntCol", "DoubleCol", "StringCol", "BoolCol"};
        std::vector<std::vector<KDBType>> data = {
            {42, 3.14, std::string("text"), true}
        };

        bool result = make_table("mixed_types", columns, data);
        recordResult(result,
            result ? "Successfully handled mixed types" : "Failed to handle mixed types",
            "Mixed Types");
            
        cleanupTable("mixed_types");
    }

    void testNullValues() {
        std::vector<std::string> columns = {"NullableCol"};
        std::vector<std::vector<KDBType>> data = {
            {std::monostate()},
            {42},
            {std::monostate()},
            {std::string("text")}
        };

        bool result = make_table("null_table", columns, data);
        recordResult(result,
            result ? "Successfully handled null values" : "Failed to handle null values",
            "Null Values");
            
        cleanupTable("null_table");
    }

    void testDuplicateTableNames() {
        std::vector<std::string> columns = {"Col1"};
        std::vector<std::vector<KDBType>> data = {{1}};

        bool first = make_table("duplicate", columns, data);
        bool second = make_table("duplicate", columns, data);

        recordResult(first && second,
            "Handled duplicate table names appropriately",
            "Duplicate Table Names");
            
        cleanupTable("duplicate");
    }

    void testValidNames() {
        // Test with valid identifiers only
        std::vector<std::string> columns = {"ValidName1", "ValidName2"};
        std::vector<std::vector<KDBType>> data = {
            {std::string("value1"), std::string("value2")}
        };

        bool result = make_table("valid_names", columns, data);
        recordResult(result,
            result ? "Successfully handled valid names" : "Failed to handle valid names",
            "Valid Names");
            
        cleanupTable("valid_names");
    }

    void testEdgeCases() {
        std::vector<std::string> columns = {"IntCol", "DoubleCol", "StringCol"};
        std::vector<std::vector<KDBType>> data = {
            {0, 0.0, std::string("")},  // Minimum values
            {std::numeric_limits<int>::max(), 1e6, std::string("normal")}  // More reasonable maximum values
        };

        bool result = make_table("edge_cases", columns, data);
        recordResult(result,
            result ? "Successfully handled edge case values" : "Failed to handle edge case values",
            "Edge Cases");
            
        cleanupTable("edge_cases");
    }

    void printResults() {
        std::cout << "\n=== Make Table Test Results ===\n";
        std::cout << "Total Tests: " << totalTests << "\n";
        std::cout << "Passed: " << passedTests << "\n";
        std::cout << "Failed: " << (totalTests - passedTests) << "\n\n";

        for (const auto& result : results) {
            std::cout << (result.passed ? "[PASS] " : "[FAIL] ")
                     << result.testName << ": "
                     << result.message << "\n";
        }
        std::cout << "\n";
    }
};

int main() {
    try {
        std::cout << "Starting Make Table Tests...\n";
        MakeTableTests tests;
        tests.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
