#include "inline_query.h"

/**
 * @brief Executes an inline KDB+ query and returns the result.
 *
 * This function sends a query string to the KDB+ server using the established
 * connection handle. It handles various response types and manages memory
 * appropriately for each case:
 * - Success with data: Returns K object (caller must free)
 * - Success without data (assignments/void): Returns true
 * - Failures (null/errors): Returns false
 *
 * @param query The KDB+ query string to be executed
 * @return QueryResult Object containing either:
 *         - K object (for successful queries returning data)
 *         - bool true (for successful queries without data)
 *         - bool false (for errors)
 * @throws May throw std::runtime_error if connection is lost
 */
QueryResult inline_query(const std::string& query) {
    // Log the query being executed for debugging purposes
    // std::cout << "Executing KDB+ query: " << query << std::endl;
    
    try {
        // Execute the query using the KDB+ handle and retrieve the result
        K result = k(KDBConnection::getHandle(), const_cast<char*>(query.c_str()), (K)0);

        // Check if the result is null, indicating a failure in execution
        if (!result) {
            std::cerr << "Query execution failed: null result" << std::endl;
            return false; // Return a QueryResult containing 'false' to indicate failure
        }

        // Handle error responses from KDB+ (type -128 indicates an error)
        if (result->t == -128) {
            std::string error_msg = result->s;  // Extract the error message
            r0(result);  // Release the K object to prevent memory leaks
            std::cerr << "Query execution error: " << error_msg << std::endl;
            return false; // Return a QueryResult containing 'false' to indicate error
        }

        // Handle successful execution with a null result (e.g., assignments or void operations)
        if (result->t == 101) {
            //std::cout << "Query executed successfully (null result due to assignment or void operation)." << std::endl;
            r0(result);  // Release the K object as there's no data to return
            return true; // Return a QueryResult containing 'true' to indicate success without data
        }

        // If execution is successful and returns data, log the success and return the result
        //std::cout << "Query executed successfully." << std::endl;
        return result; // Return a QueryResult containing the K object with query data
    }
    catch (const std::exception& e) {
        // Catch and log any exceptions that occur during query execution
        std::cerr << "Error executing query: " << e.what() << std::endl;
        return false; // Return a QueryResult containing 'false' to indicate exception
    }
}
