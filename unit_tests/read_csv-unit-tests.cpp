#include "k.h"
#include "type_map.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "inline_query.h"
#include "read_csv.h"

class TestResult {
public:
    bool passed;
    std::string message;
    std::string testName;
    
    TestResult(bool p, const std::string& msg, const std::string& name)
        : passed(p), message(msg), testName(name) {}
};

class ReadCSVTests {
private:
    std::vector<TestResult> results;
    int totalTests = 0;
    int passedTests = 0;
    
    std::string current_path = std::filesystem::current_path().string();
    const std::string TEST_DATA_DIR = current_path+"/unit_tests/test_data/";
    
    void recordResult(bool passed, const std::string& message, const std::string& testName) {
        results.emplace_back(passed, message, testName);
        totalTests++;
        if (passed) passedTests++;
    }

    bool checkFileExists(const std::string& filepath) {
        std::ifstream f(filepath);
        if (!f.good()) {
            std::cerr << "File does not exist: " << filepath << std::endl;
            return false;
        }
        f.close();
        return true;
    }

    bool verifyTableData(const std::string& tableName, size_t expectedRows) {
            try {
                // Check if table exists using tables[] command
                auto exists_result = inline_query("tables[]");
                if (!exists_result) {
                    std::cerr << "Failed to get tables list" << std::endl;
                    return false;
                }

                auto exists = exists_result.get_result();
                if (!exists) {
                    std::cerr << "Got null result from tables list" << std::endl;
                    return false;
                }

                // Get row count directly - if table doesn't exist, this will fail
                auto count_result = inline_query("count " + tableName);
                if (!count_result) {
                    std::cerr << "Failed to get row count" << std::endl;
                    return false;
                }

                K count = count_result.get_result();
                if (!count || count->t != -KJ || count->j != expectedRows) {
                    std::cerr << "Row count mismatch. Expected: " << expectedRows
                             << ", Got: " << (count ? std::to_string(count->j) : "null") << std::endl;
                    return false;
                }

                // Extra validation: Try to select from table
                auto select_result = inline_query("select from " + tableName);
                if (!select_result) {
                    std::cerr << "Failed to select from table" << std::endl;
                    return false;
                }

                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error verifying table data: " << e.what() << std::endl;
                return false;
            }
        }

    void cleanupTable(const std::string& tableName) {
        
    }

public:
    void runAllTests() {
        if (!KDBConnection::connect("localhost", 6000)) {
            std::cerr << "Failed to connect to KDB+ server" << std::endl;
            return;
        }

        try {
            testBasicCSVRead();
            testMixedTypes();
            testMissingValues();
            testSpecialCharacters();
            testPipeDelimited();
            testInvalidFilePath();
            testKeyColumn();	
            testNoHeader();
            testDuplicateTableNames();
        } catch (const std::exception& e) {
            std::cerr << "Test execution error: " << e.what() << std::endl;
        }

        KDBConnection::disconnect();
        printResults();
    }

    void testBasicCSVRead() {
        std::string filepath = TEST_DATA_DIR + "basic_data.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: basic_data.csv", "Basic CSV Read");
            return;
        }

        cleanupTable("basic_test");
        bool result = read_csv("basic_test", filepath, true);
        bool verified = result && verifyTableData("basic_test", 4);
        
        recordResult(verified,
            verified ? "Successfully read basic CSV" : "Failed to read basic CSV",
            "Basic CSV Read");
            
        cleanupTable("basic_test");
    }

    void testMixedTypes() {
        std::string filepath = TEST_DATA_DIR + "mixed_types.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: mixed_types.csv", "Mixed Types");
            return;
        }

        cleanupTable("mixed_test");
        bool result = read_csv("mixed_test", filepath, true);
        bool verified = result && verifyTableData("mixed_test", 3);
        
        recordResult(verified,
            verified ? "Successfully handled mixed types" : "Failed with mixed types",
            "Mixed Types");
            
        cleanupTable("mixed_test");
    }

    void testMissingValues() {
        std::string filepath = TEST_DATA_DIR + "missing_vals.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: missing_vals.csv", "Missing Values");
            return;
        }

        cleanupTable("missing_test");
        bool result = read_csv("missing_test", filepath, true);
        bool verified = result && verifyTableData("missing_test", 4);
        
        recordResult(verified,
            verified ? "Successfully handled missing values" : "Failed with missing values",
            "Missing Values");
            
        cleanupTable("missing_test");
    }

    void testSpecialCharacters() {
        std::string filepath = TEST_DATA_DIR + "special_chars.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: special_chars.csv", "Special Characters");
            return;
        }

        cleanupTable("special_test");
        bool result = read_csv("special_test", filepath, true);
        bool verified = result && verifyTableData("special_test", 3);
        
        recordResult(verified,
            verified ? "Successfully handled special characters" : "Failed with special characters",
            "Special Characters");
            
        cleanupTable("special_test");
    }

    void testPipeDelimited() {
        std::string filepath = TEST_DATA_DIR + "pipe_delimiter.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: pipe_delimiter.csv", "Pipe Delimiter");
            return;
        }

        cleanupTable("pipe_test");
        bool result = read_csv("pipe_test", filepath, true, '|');
        bool verified = result && verifyTableData("pipe_test", 3);
        
        recordResult(verified,
            verified ? "Successfully handled pipe delimiter" : "Failed with pipe delimiter",
            "Pipe Delimiter");
            
        cleanupTable("pipe_test");
    }

    void testInvalidFilePath() {
        std::string filepath = TEST_DATA_DIR + "nonexistent.csv";
        bool result = read_csv("invalid_test", filepath, true);
        
        recordResult(!result,
            !result ? "Correctly handled invalid file path" : "Failed to handle invalid file path",
            "Invalid File Path");
    }

    void testKeyColumn() {
        std::string filepath = TEST_DATA_DIR + "mixed_types.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: mixed_types.csv", "Key Column");
            return;
        }

        cleanupTable("key_test");
        bool result = read_csv("key_test", filepath, true, ',', "ID");
        bool verified = result && verifyTableData("key_test", 3);
        
        recordResult(verified,
            verified ? "Successfully handled key column" : "Failed with key column",
            "Key Column");
            
        cleanupTable("key_test");
    }

    void testNoHeader() {
        std::string filepath = TEST_DATA_DIR + "basic_data.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: basic_data.csv", "No Header");
            return;
        }

        cleanupTable("no_header_test");
        bool result = read_csv("no_header_test", filepath, false);
        bool verified = result && verifyTableData("no_header_test", 4);  // 5 because header becomes a row
        
        recordResult(verified,
            verified ? "Successfully handled no header case" : "Failed with no header case",
            "No Header");
            
        cleanupTable("no_header_test");
    }

    void testDuplicateTableNames() {
        std::string filepath = TEST_DATA_DIR + "basic_data.csv";
        if (!checkFileExists(filepath)) {
            recordResult(false, "File not found: basic_data.csv", "Duplicate Table Names");
            return;
        }

        cleanupTable("duplicate_test");
        bool first = read_csv("duplicate_test", filepath, true);
        bool second = read_csv("duplicate_test", filepath, true);
        
        recordResult(first && second,
            first && second ? "Successfully handled duplicate table names" : "Failed with duplicate table names",
            "Duplicate Table Names");
            
        cleanupTable("duplicate_test");
    }

    void printResults() {
        std::cout << "\n=== Read CSV Test Results ===\n";
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
        std::cout << "Starting Read CSV Tests...\n";
        ReadCSVTests tests;
        tests.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

