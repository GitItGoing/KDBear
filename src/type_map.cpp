#include "type_map.h"
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <ctime>
#include "k.h"

#ifndef ne
#define ne std::numeric_limits<float>::quiet_NaN()  ///< Define NaN for float operations if not already defined.
#endif

#include <iomanip>
#include <iostream>

namespace detail {

/**
 * @brief Parses a date string in "YYYY-MM-DD" format to a kdb+ integer date.
 * @param s The input date string.
 * @return I The integer representation of the date (days since 2000-01-01) or `ni` if parsing fails.
 */
I parse_date(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) return ni;
    std::time_t time = std::mktime(&tm);
    if (time == -1) return ni;
    return static_cast<I>((time - 946684800LL) / 86400);
}

/**
 * @brief Parses a datetime string in "YYYY-MM-DD HH:MM:SS" format to a kdb+ float datetime.
 * @param s The input datetime string.
 * @return F The float representation of the datetime (days since 2000-01-01) or `nf` if parsing fails.
 */
F parse_datetime(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) return nf;
    std::time_t time = std::mktime(&tm);
    if (time == -1) return nf;
    return static_cast<F>((time - 946684800LL) / 86400.0);
}

/**
 * @brief Parses a time string in "HH:MM:SS" format to a kdb+ integer time in milliseconds.
 * @param s The input time string.
 * @return I The integer representation of time in milliseconds or `ni` if parsing fails.
 */
I parse_time(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%H:%M:%S");
    if (ss.fail()) return ni;
    return (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec) * 1000;
}

}  // namespace detail

// Validation helper functions
namespace {

/**
 * @brief Validates if a string represents an integer.
 * @param s The input string.
 * @return bool True if the string represents a valid integer, false otherwise.
 */
bool is_integer(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    std::strtol(s.c_str(), &end, 10);
    return end != s.c_str() && *end == '\0';
}

/**
 * @brief Validates if a string represents a float.
 * @param s The input string.
 * @return bool True if the string represents a valid float, false otherwise.
 */
bool is_float(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

/**
 * @brief Validates if a string represents a boolean value.
 * Acceptable values are case-insensitive "true", "false", "1", or "0".
 * @param s The input string.
 * @return bool True if the string represents a valid boolean, false otherwise.
 */
bool is_boolean(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "true" || lower == "false" || lower == "1" || lower == "0");
}

/**
 * @brief Validates if a string matches the "YYYY-MM-DD" date format.
 * @param s The input string.
 * @return bool True if the string matches the date format, false otherwise.
 */
bool is_date(const std::string& s) {
    static const std::regex date_regex(R"(\d{4}-\d{2}-\d{2})");
    return std::regex_match(s, date_regex);
}

/**
 * @brief Validates if a string matches the "YYYY-MM-DD HH:MM:SS" datetime format.
 * Accepts both "T" and space as the separator between date and time.
 * @param s The input string.
 * @return bool True if the string matches the datetime format, false otherwise.
 */
bool is_datetime(const std::string& s) {
    static const std::regex datetime_regex(
        R"(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(\.\d+)?)"
    );
    return std::regex_match(s, datetime_regex);
}

/**
 * @brief Validates if a string matches the "HH:MM:SS" time format.
 * @param s The input string.
 * @return bool True if the string matches the time format, false otherwise.
 */
bool is_time(const std::string& s) {
    static const std::regex time_regex(R"(\d{2}:\d{2}:\d{2}(\.\d+)?)");
    return std::regex_match(s, time_regex);
}

/**
 * @brief Validates if a string matches the "YYYY.MM.DD D HH:MM:SS.fffffffff" timestamp format.
 * This format is specific to kdb+.
 * @param s The input string.
 * @return bool True if the string matches the timestamp format, false otherwise.
 */
bool is_timestamp(const std::string& s) {
    static const std::regex timestamp_regex(
        R"(\d{4}\.\d{2}\.\d{2}D\d{2}:\d{2}:\d{2}\.\d{9})"
    );
    return std::regex_match(s, timestamp_regex);
}

/**
 * @brief Validates if a string matches the "YYYY.MM" month format.
 * This format is specific to kdb+.
 * @param s The input string.
 * @return bool True if the string matches the month format, false otherwise.
 */
bool is_month(const std::string& s) {
    static const std::regex month_regex(R"(\d{4}\.\d{2}m)");
    return std::regex_match(s, month_regex);
}

/**
 * @brief Validates if a string matches the "D HH:MM:SS.fffffffff" timespan format.
 * This format is specific to kdb+.
 * @param s The input string.
 * @return bool True if the string matches the timespan format, false otherwise.
 */
bool is_timespan(const std::string& s) {
    static const std::regex timespan_regex(R"(\d+D\d{2}:\d{2}:\d{2}\.\d{9})");
    return std::regex_match(s, timespan_regex);
}

/**
 * @brief Validates if a string matches the "HH:MM" minute format.
 * @param s The input string.
 * @return bool True if the string matches the minute format, false otherwise.
 */
bool is_minute(const std::string& s) {
    static const std::regex minute_regex(R"(\d{2}:\d{2})");
    return std::regex_match(s, minute_regex);
}

/**
 * @brief Validates if a string matches the "HH:MM:SS" second format.
 * @param s The input string.
 * @return bool True if the string matches the second format, false otherwise.
 */
bool is_second(const std::string& s) {
    static const std::regex second_regex(R"(\d{2}:\d{2}:\d{2})");
    return std::regex_match(s, second_regex);
}

// Parsing helper functions

/**
 * @brief Parses a date string into a kdb+ integer date using `detail::parse_date`.
 * @param s The input date string in "YYYY-MM-DD" format.
 * @return I The kdb+ integer date or `ni` if parsing fails.
 */
I parse_date(const std::string& s) {
    return detail::parse_date(s);
}

/**
 * @brief Parses a datetime string into a kdb+ float datetime using `detail::parse_datetime`.
 * @param s The input datetime string in "YYYY-MM-DD HH:MM:SS" format.
 * @return F The kdb+ float datetime or `nf` if parsing fails.
 */
F parse_datetime(const std::string& s) {
    return detail::parse_datetime(s);
}

/**
 * @brief Parses a time string into a kdb+ integer time using `detail::parse_time`.
 * @param s The input time string in "HH:MM:SS" format.
 * @return I The kdb+ integer time or `ni` if parsing fails.
 */
I parse_time(const std::string& s) {
    return detail::parse_time(s);
}

}  // namespace
/**
 * @brief Creates an extended type map containing metadata for all supported kdb+ types.
 * This map provides validators, null assigners, value assigners, and formatters for each type.
 *
 * The map is keyed by a single-character string identifier for the type (e.g., "b" for boolean, "i" for integer).
 * Each entry maps to a `TypeInfo` structure that encapsulates type-specific operations and metadata.
 *
 * @return std::unordered_map<std::string, TypeInfo> The extended type map.
 */
std::unordered_map<std::string, TypeInfo> createExtendedTypeMap() {
    std::unordered_map<std::string, TypeInfo> type_map;

    // Boolean type validator
    auto validate_boolean = [](const std::string& s) -> bool {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return (lower == "true" || lower == "false" || lower == "1" || lower == "0");
    };

    // Integer type validator
    auto validate_integer = [](const std::string& s) -> bool {
        if (s.empty()) return false;
        char* end = nullptr;
        std::strtol(s.c_str(), &end, 10);
        return end != s.c_str() && *end == '\0';
    };

    // Float type validator
    auto validate_float = [](const std::string& s) -> bool {
        if (s.empty()) return false;
        char* end = nullptr;
        std::strtod(s.c_str(), &end);
        return end != s.c_str() && *end == '\0';
    };

    // Date type validator (YYYY-MM-DD format)
    auto validate_date = [](const std::string& s) -> bool {
        static const std::regex date_regex(R"(\d{4}-\d{2}-\d{2})");
        return std::regex_match(s, date_regex);
    };

    // Datetime type validator (YYYY-MM-DD HH:MM:SS or YYYY-MM-DDTHH:MM:SS format)
    auto validate_datetime = [](const std::string& s) -> bool {
        static const std::regex datetime_regex(
            R"(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(\.\d+)?)"
        );
        return std::regex_match(s, datetime_regex);
    };

    // Boolean type metadata
    type_map["b"] = TypeInfo{
        .kdb_type = KB,
        .name = "boolean",
        .type_char = 'b',
        .validator = validate_boolean,
        .null_assigner = [](K k, size_t idx) { kG(k)[idx] = 0; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            std::string lower = v;
            // Trim whitespace first
            lower.erase(0, lower.find_first_not_of(" \n\r\t"));
            lower.erase(lower.find_last_not_of(" \n\r\t") + 1);
            // Convert to lowercase
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            // More permissive TRUE values
            kG(k)[idx] = (lower == "true" || lower == "1" ||
                          lower == "t" || lower == "yes" ||
                          lower == "y") ? 1 : 0;
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return kG(k)[idx] ? "true" : "false";
        }
    };

    // Byte type metadata
    type_map["g"] = TypeInfo{
        .kdb_type = KG,
        .name = "byte",
        .type_char = 'x',
        .validator = [](const std::string& s) { return s.length() == 1; },
        .null_assigner = [](K k, size_t idx) { kG(k)[idx] = 0; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kG(k)[idx] = static_cast<G>(v[0]);
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return std::to_string(static_cast<int>(kG(k)[idx]));
        }
    };

    // Short type metadata
    type_map["h"] = TypeInfo{
        .kdb_type = KH,
        .name = "short",
        .type_char = 'h',
        .validator = validate_integer,
        .null_assigner = [](K k, size_t idx) { kH(k)[idx] = nh; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kH(k)[idx] = static_cast<H>(std::stoi(v));
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return std::to_string(kH(k)[idx]);
        }
    };

    // Integer type metadata
    type_map["i"] = TypeInfo{
        .kdb_type = KI,
        .name = "int",
        .type_char = 'i',
        .validator = validate_integer,
        .null_assigner = [](K k, size_t idx) { kI(k)[idx] = ni; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kI(k)[idx] = std::stoi(v);
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return std::to_string(kI(k)[idx]);
        }
    };

    // Long type metadata
    type_map["j"] = TypeInfo{
        .kdb_type = KJ,
        .name = "long",
        .type_char = 'j',
        .validator = validate_integer,
        .null_assigner = [](K k, size_t idx) { kJ(k)[idx] = nj; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kJ(k)[idx] = std::stoll(v);
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return std::to_string(kJ(k)[idx]);
        }
    };

    // Real type metadata
    type_map["e"] = TypeInfo{
        .kdb_type = KE,
        .name = "real",
        .type_char = 'e',
        .validator = validate_float,
        .null_assigner = [](K k, size_t idx) { kE(k)[idx] = ne; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kE(k)[idx] = std::stof(v);
        },
        .formatter = [](K k, size_t idx) -> std::string {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(7) << kE(k)[idx];
            return oss.str();
        }
    };

    // Float type metadata
    type_map["f"] = TypeInfo{
        .kdb_type = KF,
        .name = "float",
        .type_char = 'f',
        .validator = validate_float,
        .null_assigner = [](K k, size_t idx) { kF(k)[idx] = nf; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kF(k)[idx] = std::stod(v);
        },
        .formatter = [](K k, size_t idx) -> std::string {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(7) << kF(k)[idx];
            return oss.str();
        }
    };

    // Char type metadata
    type_map["c"] = TypeInfo{
        .kdb_type = KC,
        .name = "char",
        .type_char = 'c',
        .validator = [](const std::string& s) { return s.length() == 1; },
        .null_assigner = [](K k, size_t idx) { kC(k)[idx] = ' '; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kC(k)[idx] = v.empty() ? ' ' : v[0];
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return std::string(1, kC(k)[idx]);
        }
    };

    // Date type metadata
    type_map["d"] = TypeInfo{
        .kdb_type = KD,  ///< KDB+ type identifier for date
        .name = "date",
        .type_char = 'd',
        .validator = is_date,  ///< Uses the is_date validation function
        .null_assigner = [](K k, size_t idx) { kI(k)[idx] = ni; },  ///< Assigns `ni` for null date
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kI(k)[idx] = parse_date(v);  ///< Parses and assigns the date value
        },
        .formatter = [](K k, size_t idx) -> std::string {
            int days = kI(k)[idx];
            if (days == ni) return "NULL";  ///< Returns "NULL" for null dates
            std::time_t time = static_cast<std::time_t>(days * 86400LL) + 946684800LL;  ///< Converts days to Unix timestamp (assuming epoch 2000-01-01)
            std::tm* tm = std::gmtime(&time);
            std::ostringstream oss;
            oss << std::put_time(tm, "%Y-%m-%d");  ///< Formats the date as "YYYY-MM-DD"
            return oss.str();
        }
    };

    // Datetime type metadata
    type_map["z"] = TypeInfo{
        .kdb_type = KZ,  ///< KDB+ type identifier for datetime
        .name = "datetime",
        .type_char = 'z',
        .validator = is_datetime,  ///< Uses the is_datetime validation function
        .null_assigner = [](K k, size_t idx) { kF(k)[idx] = nf; },  ///< Assigns `nf` for null datetime
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kF(k)[idx] = parse_datetime(v);  ///< Parses and assigns the datetime value
        },
        .formatter = [](K k, size_t idx) -> std::string {
            double days = kF(k)[idx];
            if (days == nf) return "NULL";  ///< Returns "NULL" for null datetime
            std::time_t time = static_cast<std::time_t>(days * 86400.0) + 946684800LL;  ///< Converts days to Unix timestamp (assuming epoch 2000-01-01)
            std::tm* tm = std::gmtime(&time);
            std::ostringstream oss;
            oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");  ///< Formats the datetime as "YYYY-MM-DD HH:MM:SS"
            return oss.str();
        }
    };

    // Time type metadata
    type_map["t"] = TypeInfo{
        .kdb_type = KT,  ///< KDB+ type identifier for time
        .name = "time",
        .type_char = 't',
        .validator = is_time,  ///< Uses the is_time validation function
        .null_assigner = [](K k, size_t idx) { kI(k)[idx] = ni; },  ///< Assigns `ni` for null time
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kI(k)[idx] = parse_time(v);  ///< Parses and assigns the time value in milliseconds
        },
        .formatter = [](K k, size_t idx) -> std::string {
            int milliseconds = kI(k)[idx];
            if (milliseconds == ni) return "NULL";  ///< Returns "NULL" for null time
            int total_seconds = milliseconds / 1000;
            int hours = total_seconds / 3600;
            int minutes = (total_seconds % 3600) / 60;
            int seconds = total_seconds % 60;
            std::ostringstream oss;
            oss << std::setw(2) << std::setfill('0') << hours << ":"
                << std::setw(2) << std::setfill('0') << minutes << ":"
                << std::setw(2) << std::setfill('0') << seconds;  ///< Formats the time as "HH:MM:SS"
            return oss.str();
        }
        };
    
    // Symbol type metadata
    type_map["s"] = TypeInfo{
        .kdb_type = KS,
        .name = "symbol",
        .type_char = 's',
        .validator = nullptr,  // Symbols accept any string
        .null_assigner = [](K k, size_t idx) { kS(k)[idx] = nullptr; },
        .value_assigner = [](K k, const std::string& v, size_t idx) {
            kS(k)[idx] = ss((S)v.c_str());
        },
        .formatter = [](K k, size_t idx) -> std::string {
            return kS(k)[idx] ? std::string(kS(k)[idx]) : "";
        }
    };


    return type_map;
}

/**
 * @brief Retrieves the extended type map containing metadata for all supported kdb+ types.
 * This function initializes the type map only once and reuses it across subsequent calls.
 *
 * @return const std::unordered_map<std::string, TypeInfo>& A reference to the extended type map.
 */
const std::unordered_map<std::string, TypeInfo>& getExtendedTypeMap() {
    static const auto type_map = createExtendedTypeMap();
    return type_map;
}

/**
 * @brief Checks whether a value in a kdb+ column is null
 * @param col_data The kdb+ column (K object)
 * @param idx The index of the value to check
 * @return bool True if the value is null, false otherwise
 * @note Symbol type (KS) has special null handling with nullptr check
 * @note Returns true for unrecognized types as a safety measure
 */
bool is_null_value(K col_data, size_t idx) {
    auto type_map = getExtendedTypeMap();

    // Iterate through the type map to find the matching type handler
    for (const auto& [key, info] : type_map) {
        if (info.kdb_type == col_data->t) {
            // Handle nulls for symbol type separately
            if (col_data->t == KS) return kS(col_data)[idx] == nullptr;
            return info.formatter(col_data, idx) == "NULL";
        }
    }
    return true;  // Default to true if type is unrecognized
}

/**
 * @brief Assigns a null value to a specific index in a kdb+ column.
 * Uses the type-specific null_assigner function from the `TypeInfo` structure.
 *
 * @param col_data The kdb+ column (K object).
 * @param idx The index to assign a null value to.
 */
void assign_null_value(K col_data, size_t idx) {
    auto type_map = getExtendedTypeMap();

    // Find the appropriate type handler and assign null
    for (const auto& [key, info] : type_map) {
        if (info.kdb_type == col_data->t) {
            info.null_assigner(col_data, idx);
            return;
        }
    }
}

/**
 * @brief Assigns a value from a string to a specific index in a kdb+ column
 * @param col_data The kdb+ column (K object)
 * @param value The string representation of the value to assign
 * @param idx The index to assign the value to
 * @note Assigns null value if string is empty or conversion fails
 * @note Uses type-specific value_assigner from TypeInfo structure
 */
void assign_value(K col_data, const std::string& value, size_t idx) {
    if (value.empty()) {
        // Assign null if the value is empty
        assign_null_value(col_data, idx);
        return;
    }

    auto type_map = getExtendedTypeMap();

    // Find the appropriate type handler and assign the value
    for (const auto& [key, info] : type_map) {
        if (info.kdb_type == col_data->t) {
            try {
                info.value_assigner(col_data, value, idx);
            } catch (...) {
                // Assign null if an exception occurs during assignment
                assign_null_value(col_data, idx);
            }
            return;
        }
    }
}

/**
 * @brief Formats a value in a kdb+ column as a string.
 * Handles null pointers, special K values, and type-specific formatting.
 *
 * @param col_data The kdb+ column (K object).
 * @param idx The index of the value to format.
 * @return std::string The formatted string representation of the value.
 */
std::string format_value(K col_data, size_t idx) {
    // Debug print for diagnostics (optional, can be removed in production)
    std::cerr << "Got K object: " << (void*)col_data
              << " idx: " << idx
              << " isNull: " << (col_data == nullptr)
              << " isK0: " << (col_data == (K)0)
              << " isK1: " << (col_data == (K)-1)
              << std::endl;

    // Handle null pointers or special K values
    if (!col_data || col_data == (K)0 || col_data == (K)-1) {
        return "NULL";
    }

    auto type_map = getExtendedTypeMap();

    // Find the appropriate type handler and format the value
    for (const auto& [key, info] : type_map) {
        if (info.kdb_type == col_data->t) {
            return info.formatter(col_data, idx);
        }
    }
    return "NULL";  // Default to "NULL" if type is unrecognized
}

/**
 * @brief Infers the kdb+ type of a column based on its data
 * @param data Vector of strings representing the column's values
 * @return I The inferred kdb+ type code
 * @note Uses a priority order for type inference:
 *       boolean -> integer -> long -> float -> date -> datetime ->
 *       time -> timestamp -> month -> timespan -> minute -> second -> symbol
 * @note Returns symbol type (KS) if no other type matches
 */
I infer_column_type(const std::vector<std::string>& data) {
    auto type_map = getExtendedTypeMap();

    // Priority order for type inference
    std::vector<std::string> type_priority = {
        "b", "i", "j", "f", "d", "z", "t", "p", "m", "n", "u", "v", "s"
    };

    bool has_non_empty = false;
    std::unordered_map<std::string, bool> type_validity;

    // Initialize all types as valid
    for (const auto& [key, _] : type_map) {
        type_validity[key] = true;
    }

    // Test each value against each type's validator
    for (const auto& value : data) {
        if (!value.empty()) {
            has_non_empty = true;
            for (const auto& [key, info] : type_map) {
                if (info.validator && type_validity[key]) {
                    type_validity[key] = info.validator(value);
                }
            }
        }
    }

    // Return the first valid type according to priority
    for (const auto& type : type_priority) {
        if (type_validity[type] && has_non_empty) {
            return type_map[type].kdb_type;
        }
    }

    // Default to symbol type if no other type matches
    return type_map["s"].kdb_type;
}

/**
 * @brief Retrieves a simplified type map for backward compatibility.
 * Maps type strings to their kdb+ type codes and human-readable names.
 *
 * @return std::unordered_map<std::string, std::pair<int, std::string>> The simplified type map.
 */
std::unordered_map<std::string, std::pair<int, std::string>> getTypeMap() {
    std::unordered_map<std::string, std::pair<int, std::string>> result;
    auto extended_map = getExtendedTypeMap();

    // Populate the simplified map with type codes and names
    for (const auto& [key, info] : extended_map) {
        result[key] = {info.kdb_type, info.name};
    }

    return result;
}
