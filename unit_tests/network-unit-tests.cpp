#include "connections.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <sstream>

class TestResult {
public:
    bool passed;
    std::string message;
    std::string testName;
    
    TestResult(bool p, const std::string& msg, const std::string& name)
        : passed(p), message(msg), testName(name) {}
};

class ConnectionTests {
private:
    std::vector<TestResult> results;
    int totalTests = 0;
    int passedTests = 0;

    void recordResult(bool passed, const std::string& message, const std::string& testName) {
        results.emplace_back(passed, message, testName);
        totalTests++;
        if (passed) passedTests++;
    }

    // Helper to check if any KDB+ related resources are leaked
    bool verifyNoResourceLeak() {
        // Try to create a new connection - should succeed if no resources are leaked
        bool canConnect = KDBConnection::connect("localhost", 6000);
        if (canConnect) {
            KDBConnection::disconnect();
        }
        return canConnect;
    }

public:
    void runAllTests() {
        testConnectionInitialization();
        testSingletonBehavior();
        testDisconnectBehavior();
        testReconnectBehavior();
        testConnectionFailure();
        testConnectionHandle();
        testResourceCleanup();
        testConcurrentAccess();
        testErrorConditions();
        testAutoCleanup();
        printResults();
    }

    void testConnectionInitialization() {
        // Test initial connection
        bool result = KDBConnection::connect("localhost", 6000);
        recordResult(result,
            result ? "Successfully established initial connection" : "Failed to establish initial connection",
            "Connection Initialization");
        
        if (result) {
            KDBConnection::disconnect();
        }
    }

    void testSingletonBehavior() {
        // First connection
        bool firstConn = KDBConnection::connect("localhost", 6000);
        
        // Second connection attempt should return true (already connected)
        bool secondConn = KDBConnection::connect("localhost", 6000);
        
        // Test singleton behavior
        bool singletonWorks = firstConn && secondConn;
        
        recordResult(singletonWorks,
            singletonWorks ? "Singleton pattern working correctly" : "Singleton pattern failed",
            "Singleton Behavior");
        
        KDBConnection::disconnect();
    }

    void testDisconnectBehavior() {
        // Connect and verify
        bool connected = KDBConnection::connect("localhost", 6000);
        if (!connected) {
            recordResult(false, "Failed to connect for disconnect test", "Disconnect Behavior");
            return;
        }

        // Disconnect
        KDBConnection::disconnect();
        
        // Try to get handle - should throw
        bool throwsAfterDisconnect = false;
        try {
            KDBConnection::getHandle();
        } catch (const std::runtime_error&) {
            throwsAfterDisconnect = true;
        }

        recordResult(throwsAfterDisconnect,
            throwsAfterDisconnect ? "Disconnect behavior correct" : "Disconnect behavior incorrect",
            "Disconnect Behavior");
    }

    void testReconnectBehavior() {
        // Connect
        bool firstConnect = KDBConnection::connect("localhost", 6000);
        if (!firstConnect) {
            recordResult(false, "Initial connection failed", "Reconnect Behavior");
            return;
        }

        // Disconnect
        KDBConnection::disconnect();

        // Reconnect
        bool reconnected = KDBConnection::connect("localhost", 6000);
        
        recordResult(reconnected,
            reconnected ? "Successfully reconnected" : "Failed to reconnect",
            "Reconnect Behavior");

        if (reconnected) {
            KDBConnection::disconnect();
        }
    }

    void testConnectionFailure() {
        // Test connection to non-existent server
        bool failedConn = !KDBConnection::connect("nonexistent", 9999);
        
        recordResult(failedConn,
            failedConn ? "Correctly handled connection failure" : "Failed to handle connection failure",
            "Connection Failure");
    }

    void testConnectionHandle() {
        // Test handle retrieval
        bool connected = KDBConnection::connect("localhost", 6000);
        if (!connected) {
            recordResult(false, "Failed to connect for handle test", "Handle Retrieval");
            return;
        }

        bool handleValid = false;
        try {
            I handle = KDBConnection::getHandle();
            handleValid = handle > 0;
        } catch (const std::exception& e) {
            handleValid = false;
        }

        recordResult(handleValid,
            handleValid ? "Successfully retrieved valid handle" : "Failed to retrieve valid handle",
            "Handle Retrieval");

        KDBConnection::disconnect();
    }

    void testResourceCleanup() {
        // Multiple connect/disconnect cycles
        bool resourcesClean = true;
        for (int i = 0; i < 10; ++i) {
            bool connected = KDBConnection::connect("localhost", 6000);
            if (connected) {
                KDBConnection::disconnect();
            } else {
                resourcesClean = false;
                break;
            }
        }

        recordResult(resourcesClean,
            resourcesClean ? "Resource cleanup successful" : "Resource cleanup failed",
            "Resource Cleanup");
    }

    void testConcurrentAccess() {
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};
        std::atomic<int> exceptionCount{0};
        const int numThreads = 10;

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&]() {
                try {
                    if (KDBConnection::connect("localhost", 6000)) {
                        successCount++;
                        // Try to get handle
                        try {
                            I handle = KDBConnection::getHandle();
                            if (handle <= 0) exceptionCount++;
                        } catch (const std::exception&) {
                            exceptionCount++;
                        }
                    }
                } catch (const std::exception&) {
                    exceptionCount++;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        KDBConnection::disconnect();

        bool threadsHandled = (successCount == numThreads && exceptionCount == 0);
        recordResult(threadsHandled,
            "Concurrent access handled " + std::to_string(successCount) + "/" + std::to_string(numThreads) + " successfully",
            "Concurrent Access");
    }

    void testErrorConditions() {
        bool handlesErrors = true;
        
        // Test invalid port - should return false because khp() will fail
        bool invalidPortResult = KDBConnection::connect("localhost", -1);
        if (invalidPortResult) {
            handlesErrors = false;
            std::cout << "Failed: Invalid port connection succeeded when it should fail\n";
        }

        // Clean up from previous test
        KDBConnection::disconnect();

        // Test non-existent host - should return false as connection will fail
        bool nonExistentHostResult = KDBConnection::connect("nonexistent.invalid", 6000);
        if (nonExistentHostResult) {
            handlesErrors = false;
            std::cout << "Failed: Non-existent host connection succeeded when it should fail\n";
        }

        // Clean up from previous test
        KDBConnection::disconnect();

        // Test connection validation
        bool validationResult = is_connection_successful(-1);  // Should return false
        if (validationResult) {
            handlesErrors = false;
            std::cout << "Failed: Connection validation succeeded with invalid handle\n";
        }

        recordResult(handlesErrors,
            handlesErrors ? "Error conditions handled correctly" : "Error conditions not handled correctly",
            "Error Conditions");
    }

    void testAutoCleanup() {
        {
            // Create a scope for testing cleanup
            bool connected = KDBConnection::connect("localhost", 6000);
            if (!connected) {
                recordResult(false, "Failed to connect for cleanup test", "Auto Cleanup");
                return;
            }
            // Let scope end without explicit disconnect
        }

        // Verify we can establish a new connection
        bool canReconnect = verifyNoResourceLeak();
        
        recordResult(canReconnect,
            canReconnect ? "Automatic cleanup successful" : "Automatic cleanup failed",
            "Auto Cleanup");
    }

    void printResults() {
        std::cout << "\n=== KDB+ Connection Test Results ===\n";
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
        std::cout << "Starting KDB+ Connection Tests...\n";
        ConnectionTests tests;
        tests.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
