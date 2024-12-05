
#include "joins.h"
#include "inline_query.h"
#include "make_table.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>
#include "print_table.h"
namespace test {

class TestResult {
public:
    bool passed;
    std::string message;
    std::string testName;
    
    TestResult(bool p, const std::string& msg, const std::string& name)
        : passed(p), message(msg), testName(name) {}
};

class JoinsTest {
private:
    std::vector<TestResult> results;
    int totalTests = 0;
    int passedTests = 0;

    void recordResult(bool passed, const std::string& message, const std::string& testName) {
        results.emplace_back(passed, message, testName);
        totalTests++;
        if (passed) passedTests++;
    }

    // Helper function to create test tables
    bool setup_test_tables() {
        // Ensure table1 exists
        auto result = inline_query("select from table1");
        if (!bool(result)) {
            std::cerr << "table1 does not exist. Creating table1..." << std::endl;
            if (!inline_query("table1:([] ticker:`GOOG`MSFT`AAPL; price:20 30 40; size:10 20 30)")) {
                std::cerr << "Failed to create table1" << std::endl;
                return false;
            }
        }

        // Ensure table2 exists
        result = inline_query("select from table2");
        if (!bool(result)) {
            std::cerr << "table2 does not exist. Creating table2..." << std::endl;
            if (!inline_query("table2:([ticker:`GOOG`AAPL] bid:19 39; ask:22 44; size:50 40)")) {
                std::cerr << "Failed to create table2" << std::endl;
                return false;
            }
        }

        return true;
    }

    void cleanup_test_tables() {
        // No need to delete table1 and table2
        inline_query("delete test_result from `.");
    }

    // Helper function to create time-based test tables
    bool setup_time_test_tables() {
        // Create table1_time with 'time' data type
        if (!inline_query("table1_time:([] ticker:`GOOG`MSFT`AAPL; time:09:30:00.000t 09:31:00.000t 09:32:00.000t; price:20 30 40)")) {
            std::cerr << "Failed to create table1_time" << std::endl;
            return false;
        }

        // Create table2_time with 'time' data type
        if (!inline_query("table2_time:([] ticker:`GOOG`AAPL`IBM; time:09:30:30.000t 09:31:30.000t 09:33:00.000t; bid:19 39 50; ask:22 44 55)")) {
            std::cerr << "Failed to create table2_time" << std::endl;
            return false;
        }

        return true;
    }


    void cleanup_time_test_tables() {
        inline_query("delete table1_time from `.");
        inline_query("delete table2_time from `.");
        inline_query("delete test_result from `.");
    }

    bool verify_join_result(const std::string& result_name, int expected_row_count) {
        auto result = inline_query("count " + result_name);
        if (!bool(result)) {
            std::cerr << "Failed to get row count" << std::endl;
            return false;
        }

        K count_k = result.get_result();
        if (!count_k || count_k->t != -KJ) {
            std::cerr << "Invalid count result: Expected type -KJ (long), got type " << (count_k ? count_k->t : -9999) << std::endl;
            if (count_k) r0(count_k);
            return false;
        }

        long actual_count = count_k->j;
        r0(count_k);

        if (actual_count != expected_row_count) {
            std::cerr << "Expected " << expected_row_count << " rows, got "
                      << actual_count << std::endl;

            // Fetch and print the result table for debugging
            auto table_result = inline_query(result_name);
            if (bool(table_result)) {
                std::cout << "Resulting table:\n";
                print_head(table_result.get_result());
            }

            return false;
        }

        return true;
    }

public:
    bool test_inner_join_basic() {
        if (!setup_test_tables()) return false;

        std::vector<std::string> join_cols = {"ticker"};
        auto result = joins::inner_join("table1", "table2", "test_result", join_cols);

        bool success = bool(result) && verify_join_result("test_result", 2);
        cleanup_test_tables();
        return success;
    }

    bool test_left_join_basic() {
        if (!setup_test_tables()) return false;

        std::vector<std::string> join_cols = {"ticker"};
        auto result = joins::left_join("table1", "table2", "test_result", join_cols);

        bool success = bool(result) && verify_join_result("test_result", 3);
        cleanup_test_tables();
        return success;
    }

    bool test_right_join_basic() {
        if (!setup_test_tables()) return false;

        std::vector<std::string> join_cols = {"ticker"};
        auto result = joins::right_join("table1", "table2", "test_result", join_cols);

        bool success = bool(result) && verify_join_result("test_result", 2);
        cleanup_test_tables();
        return success;
    }

    bool test_union_join_basic() {
        if (!setup_test_tables()) return false;

        auto result = joins::union_join("table1", "table2", "test_result", {});

        // The expected row count is 5
        bool success = bool(result) && verify_join_result("test_result", 5);
        cleanup_test_tables();
        return success;
    }

    bool test_window_join_basic() {
        if (!setup_time_test_tables()) return false;

        std::vector<std::string> join_cols = {"ticker"};
        // Perform window join with window size of 60 seconds
        auto result = joins::window_join("table1_time", "table2_time", "test_result", "time", "time", 60.0, join_cols);

        // Expected rows are for tickers GOOG and AAPL
        bool success = bool(result) && verify_join_result("test_result", 3);

        cleanup_time_test_tables();
        return success;
    }




    bool test_asof_join_basic() {
        if (!setup_time_test_tables()) return false;

        std::vector<std::string> join_cols = {"ticker"};
        // Perform asof join on specified time columns and join columns
        auto result = joins::asof_join("table1_time", "table2_time", "test_result", "time", "time", join_cols);

        // Since asof join matches the last available timestamp less than or equal to, expected rows equal to table1_time
        bool success = bool(result) && verify_join_result("test_result", 3);

        cleanup_time_test_tables();
        return success;
    }


    void run_all_tests() {
        // Ensure we're connected to KDB+
        if (!KDBConnection::connect("localhost", 6000)) {
            std::cerr << "Failed to connect to KDB+ server" << std::endl;
            return;
        }

        std::cout << "\nRunning Join Operations Tests...\n" << std::endl;

        struct TestCase {
            const char* name;
            bool (JoinsTest::*test)();
        };

        try {
            TestCase tests[] = {
                {"Inner join basic test", &JoinsTest::test_inner_join_basic},
                {"Left join basic test", &JoinsTest::test_left_join_basic},
                {"Right join basic test", &JoinsTest::test_right_join_basic},
                {"Union join basic test", &JoinsTest::test_union_join_basic},
                {"Window join basic test", &JoinsTest::test_window_join_basic},
                {"Asof join basic test", &JoinsTest::test_asof_join_basic}, // New test added here
            };

            for (const auto& test : tests) {
                bool testResult = (this->*test.test)();
                recordResult(testResult,
                             testResult ? "Test completed successfully" : "Test failed",
                             test.name);
            }
        } catch (const std::exception& e) {
            std::cerr << "Test execution error: " << e.what() << std::endl;
        }

        KDBConnection::disconnect();
        printResults();
    }

    void printResults() {
        std::cout << "\n=== Join Operations Test Results ===\n";
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

} // namespace test

int main() {
    try {
        std::cout << "Starting Join Operations Tests...\n";
        test::JoinsTest test_suite;
        test_suite.run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

