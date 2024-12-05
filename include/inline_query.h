#ifndef INLINE_QUERY_H
#define INLINE_QUERY_H

#include "k.h"               
#include "connections.h"
#include <string>
#include <iostream>
#include <variant>

/**
 * @brief Represents the result of an inline KDB+ query.
 *
 * The `QueryResult` class encapsulates either a boolean indicating success/failure
 * or a KDB+ result object. It provides mechanisms to evaluate the success of the
 * query and access the returned data if available.
 */
class QueryResult {
public:
    // ============================================================================
    // Constructors
    // ============================================================================

    /**
     * @brief Constructs a `QueryResult` representing a boolean outcome.
     *
     * @param success A boolean indicating the success (`true`) or failure (`false`) of the query.
     */
    QueryResult(bool success) : data(success) {}

    /**
     * @brief Constructs a `QueryResult` with a KDB+ result object.
     *
     * @param result The KDB+ result object obtained from executing the query.
     */
    QueryResult(K result) : data(result) {}

    // ============================================================================
    // Operator Overloads
    // ============================================================================

    /**
     * @brief Overloads the boolean conversion operator.
     *
     * Allows the `QueryResult` object to be used in boolean contexts to check
     * the success of the query.
     *
     * @return bool `true` if the query was successful or if a KDB+ result is present;
     *               `false` otherwise.
     */
    operator bool() const {
        return std::holds_alternative<bool>(data)
                   ? std::get<bool>(data) // Return true/false for boolean
                   : true;                // Always true for K object
    }

    // ============================================================================
    // Accessor Methods
    // ============================================================================

    /**
     * @brief Retrieves the KDB+ result object if available.
     *
     * @return K The KDB+ result object if the query returned data;
     *           `nullptr` otherwise.
     */
    K get_result() const {
        return std::holds_alternative<K>(data) ? std::get<K>(data) : nullptr;
    }

private:
    std::variant<bool, K> data; ///< Holds either a boolean or a KDB+ result object.
};

// ============================================================================
 // Function Declarations
// ============================================================================

/**
 * @brief Executes an inline KDB+ query and returns the result.
 *
 * Sends a query string to the KDB+ server using the established connection handle.
 * It handles various response types, including successful execution, errors, and
 * null results due to assignments or void operations.
 *
 * @param query The KDB+ query string to be executed.
 * @return QueryResult The result of the query execution. Returns `false` if
 *         execution fails or an error occurs; otherwise, returns the result
 *         object.
 */
QueryResult inline_query(const std::string& query);

#endif // INLINE_QUERY_H
