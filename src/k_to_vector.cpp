#include "k_to_vector.h"
#include "type_map.h"
#include <iostream>
#include <stdexcept>
#include <limits>

/**
 * @brief Converts a K object value at specified index to a C++ type wrapped in KValue
 *
 * @param coldata Pointer to K object containing the data
 * @param idx Index of the value to convert
 * @return std::optional<KValue> Converted value wrapped in optional, returns nullopt if value is null
 * @throws std::invalid_argument if K type is unsupported
 */
std::optional<KValue> convert_k_value(K coldata, J idx) {
    // Return nullopt for null K objects or null values
    if (!coldata || is_null_value(coldata, idx)) {
        return std::nullopt;
    }

    switch (coldata->t) {
        // Boolean type conversion
        case KB:
            return KValue{static_cast<bool>(kG(coldata)[idx])};
        
        // Byte type conversion
        case KG:
            return KValue{static_cast<int8_t>(kG(coldata)[idx])};
        
        // Short integer type conversion
        case KH:
            return KValue{static_cast<int16_t>(kH(coldata)[idx])};
        
        // 32-bit integer type conversion
        case KI:
            return KValue{static_cast<int32_t>(kI(coldata)[idx])};
        
        // 64-bit integer type conversion
        case KJ:
            return KValue{static_cast<int64_t>(kJ(coldata)[idx])};
        
        // Single-precision float type conversion
        case KE:
            return KValue{static_cast<float>(kE(coldata)[idx])};
        
        // Double-precision float type conversion
        case KF:
            return KValue{static_cast<double>(kF(coldata)[idx])};
        
        // Character type conversion
        case KC:
            return KValue{static_cast<char>(kC(coldata)[idx])};
        
        // Symbol (string) type conversion
        case KS:
            return KValue{std::string(kS(coldata)[idx] ? kS(coldata)[idx] : "")};
        
        // Date type conversion
        case KD: {
            auto days = kI(coldata)[idx];
            if (days != ni) {
                // Convert days since 2000.01.01 to system time
                auto tp = std::chrono::system_clock::from_time_t(946684800) +
                         std::chrono::hours(24) * days;
                return KValue{KDate{tp}};
            }
            return std::nullopt;
        }
        
        // DateTime type conversion
        case KZ: {
            auto days = kF(coldata)[idx];
            if (days != nf) {
                // Convert fractional days since 2000.01.01 to system time
                auto tp = std::chrono::system_clock::from_time_t(946684800) +
                         std::chrono::seconds(static_cast<int64_t>(days * 86400.0));
                return KValue{KDateTime{tp}};
            }
            return std::nullopt;
        }
        
        default:
            throw std::invalid_argument("Unsupported K type for conversion");
    }
}

/**
 * @brief Converts a K list object to a vector of KValues
 *
 * @param obj Pointer to K list object
 * @return std::vector<std::optional<KValue>> Vector of converted values
 * @throws std::invalid_argument if K object is null
 */
std::vector<std::optional<KValue>> convert_list(K obj) {
    if (!obj) {
        throw std::invalid_argument("Null K object.");
    }

    std::vector<std::optional<KValue>> result;
    result.reserve(obj->n);
    
    // Process each element in the list
    for (J i = 0; i < obj->n; ++i) {
        if (obj->t == 0) {  // Handle mixed-type list
            result.push_back(convert_k_value(kK(obj)[i], 0));
        } else {  // Handle homogeneous list
            result.push_back(convert_k_value(obj, i));
        }
    }
    
    return result;
}

/**
 * @brief Converts a K table object to a 2D vector of KValues
 *
 * First row contains column headers as strings.
 * Subsequent rows contain the table data.
 *
 * @param obj Pointer to K table object
 * @return std::vector<std::vector<std::optional<KValue>>> 2D vector of converted values
 * @throws std::invalid_argument if input is not a valid table object
 */
std::vector<std::vector<std::optional<KValue>>> convert_table(K obj) {
    if (!obj || obj->t != XT) {
        throw std::invalid_argument("Expected a table K object.");
    }

    std::vector<std::vector<std::optional<KValue>>> result;

    // Extract dictionary containing column names and values
    K dict = obj->k;
    K colnames = kK(dict)[0];
    K colvalues = kK(dict)[1];

    // Process column headers
    std::vector<std::optional<KValue>> headers;
    headers.reserve(colnames->n);
    for (J i = 0; i < colnames->n; ++i) {
        headers.push_back(std::optional<KValue>{std::string(kS(colnames)[i])});
    }
    result.push_back(headers);

    // Calculate number of data rows and reserve space
    J row_count = colvalues->n > 0 ? kK(colvalues)[0]->n : 0;
    result.reserve(row_count + 1);  // Add 1 for headers

    // Process data rows
    for (J row = 0; row < row_count; ++row) {
        std::vector<std::optional<KValue>> row_data;
        row_data.reserve(colvalues->n);
        
        // Convert each column value in the current row
        for (J col = 0; col < colvalues->n; ++col) {
            K coldata = kK(colvalues)[col];
            if (row < coldata->n) {
                row_data.push_back(convert_k_value(coldata, row));
            } else {
                row_data.push_back(std::nullopt);
            }
        }
        result.push_back(std::move(row_data));
    }
    
    return result;
}
