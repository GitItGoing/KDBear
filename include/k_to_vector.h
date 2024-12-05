#ifndef K_TO_VECTOR_H
#define K_TO_VECTOR_H

#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <chrono>
#include <type_traits>
#include <iomanip>
#include <iostream>
#include "k.h"

/**
 * @brief Wrapper struct for K date values
 *
 * Used to distinguish date values from datetime values in the variant type
 */
struct KDate {
    std::chrono::system_clock::time_point value;
    explicit KDate(std::chrono::system_clock::time_point tp) : value(tp) {}
};

/**
 * @brief Wrapper struct for K datetime values
 *
 * Used to distinguish datetime values from date values in the variant type
 */
struct KDateTime {
    std::chrono::system_clock::time_point value;
    explicit KDateTime(std::chrono::system_clock::time_point tp) : value(tp) {}
};

/**
 * @brief Variant type representing all possible K column value types
 *
 * Maps K types to corresponding C++ types:
 * - KB -> bool
 * - KG -> int8_t
 * - KH -> int16_t
 * - KI -> int32_t
 * - KJ -> int64_t
 * - KE -> float
 * - KF -> double
 * - KC -> char
 * - KS -> std::string
 * - KD -> KDate
 * - KZ -> KDateTime
 */
using KValue = std::variant<
    bool,           // KB
    int8_t,         // KG
    int16_t,        // KH
    int32_t,        // KI
    int64_t,        // KJ
    float,          // KE
    double,         // KF
    char,           // KC
    std::string,    // KS
    KDate,          // KD
    KDateTime       // KZ
>;

// Forward declarations for implementation functions
std::vector<std::vector<std::optional<KValue>>> convert_table(K obj);
std::vector<std::optional<KValue>> convert_list(K obj);

/**
 * @brief Checks if a K object represents a table
 *
 * @param obj K object to check
 * @return true if object is a valid table, false otherwise
 */
inline bool is_table(K obj) {
    return obj && obj->t == XT;
}

// Type aliases for improved readability
using KList = std::vector<std::optional<KValue>>;
using KTable = std::vector<std::vector<std::optional<KValue>>>;
using KResult = std::variant<KList, KTable>;

/**
 * @brief Main conversion function to convert K objects to C++ vectors
 *
 * @param obj K object to convert
 * @return KResult containing either a KList or KTable
 * @throws std::invalid_argument if input is null
 */
inline KResult k_to_vector(K obj) {
    if (!obj) {
        throw std::invalid_argument("Null K object");
    }
    if (is_table(obj)) {
        return convert_table(obj);
    }
    return convert_list(obj);
}

/**
 * @brief Helper function to iterate over KResult contents
 *
 * @tparam Func Callable type that accepts either KList or KTable rows
 * @param result KResult to iterate over
 * @param func Function to apply to each row
 */
template<typename Func>
void for_each(const KResult& result, Func&& func) {
    std::visit([&func](const auto& data) {
        for(const auto& row : data) {
            func(row);
        }
    }, result);
}

/**
 * @brief Prints a single KValue to stdout
 *
 * Handles null values and special formatting for dates and datetimes
 *
 * @param opt_val Optional KValue to print
 */
inline void print_value(const std::optional<KValue>& opt_val) {
    if (!opt_val) {
        std::cout << "null";
        return;
    }
    
    std::visit([](const auto& val) {
        // Special handling for date types
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(val)>, KDate>) {
            auto time = std::chrono::system_clock::to_time_t(val.value);
            std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d");
        }
        // Special handling for datetime types
        else if constexpr (std::is_same_v<std::remove_cvref_t<decltype(val)>, KDateTime>) {
            auto time = std::chrono::system_clock::to_time_t(val.value);
            std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        }
        // Default handling for all other types
        else {
            std::cout << val;
        }
    }, *opt_val);
}

/**
 * @brief Prints a row of data to stdout
 *
 * Handles both single values (from lists) and multiple values (from tables)
 *
 * @tparam T Row type (either std::optional<KValue> or vector of such)
 * @param row Row data to print
 */
inline void print_row(const auto& row) {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(row)>, std::optional<KValue>>) {
        // Handle single value (from list)
        print_value(row);
    } else {
        // Handle row of values (from table)
        for(const auto& value : row) {
            print_value(value);
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

#endif // K_TO_VECTOR_H
