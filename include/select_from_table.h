#ifndef SELECT_FROM_TABLE_H
#define SELECT_FROM_TABLE_H

#include "k.h"
#include "connections.h"
#include <string>
#include <vector>
#include <memory>
#include <iomanip>
#include <sstream>
#include <iomanip>
#include <ctime>  

/**
 * @brief Represents a value from a KDB+ column.
 *
 * The `KDBValue` class encapsulates different KDB+ data types, providing type-safe access and conversion
 * functionalities. It supports all standard KDB+ types, including temporal types.
 */
class KDBValue {
public:
    /**
     * @enum Type
     * @brief Enumerates the possible types of KDB+ values.
     */
    enum class Type {
        Null,
        Boolean,    // -KB, KB
        Byte,       // -KG, KG
        Short,      // -KH, KH
        Integer,    // -KI, KI
        Long,       // -KJ, KJ
        Real,       // -KE, KE
        Float,      // -KF, KF
        Char,       // -KC, KC
        Symbol,     // -KS, KS
        Date,       // -KD, KD
        Month,      // -KM, KM
        Time,       // -KT, KT
        Minute,     // -KU, KU
        Second,     // -KV, KV
        DateTime,   // -KZ, KZ
        TimeSpan    // -KN, KN
    };

    /**
     * @brief Default constructor - creates a null value.
     */
    KDBValue() : type_(Type::Null) {}
    
    // Constructors for each type
    explicit KDBValue(bool val) : type_(Type::Boolean), bool_val_(val) {}
    explicit KDBValue(char val) : type_(Type::Char), char_val_(val) {}
    explicit KDBValue(unsigned char val) : type_(Type::Byte), byte_val_(val) {}
    explicit KDBValue(short val) : type_(Type::Short), short_val_(val) {}
    explicit KDBValue(int val) : type_(Type::Integer), int_val_(val) {}
    explicit KDBValue(long long val) : type_(Type::Long), long_val_(val) {}
    explicit KDBValue(float val) : type_(Type::Real), real_val_(val) {}
    explicit KDBValue(double val) : type_(Type::Float), float_val_(val) {}
    explicit KDBValue(std::string val) : type_(Type::Symbol), string_val_(std::move(val)) {}

    // Special constructors for time-based types
    static KDBValue create_date(long long days) {
        KDBValue val;
        val.type_ = Type::Date;
        val.long_val_ = days;
        return val;
    }

    static KDBValue create_month(int months) {
        KDBValue val;
        val.type_ = Type::Month;
        val.int_val_ = months;
        return val;
    }

    static KDBValue create_time(int milliseconds) {
        KDBValue val;
        val.type_ = Type::Time;
        val.int_val_ = milliseconds;
        return val;
    }

    static KDBValue create_minute(int minutes) {
        KDBValue val;
        val.type_ = Type::Minute;
        val.int_val_ = minutes;
        return val;
    }

    static KDBValue create_second(int seconds) {
        KDBValue val;
        val.type_ = Type::Second;
        val.int_val_ = seconds;
        return val;
    }

    static KDBValue create_datetime(double days) {
        KDBValue val;
        val.type_ = Type::DateTime;
        val.float_val_ = days;
        return val;
    }

    static KDBValue create_timespan(long long nanoseconds) {
        KDBValue val;
        val.type_ = Type::TimeSpan;
        val.long_val_ = nanoseconds;
        return val;
    }

    // Type checking methods
    bool is_null() const { return type_ == Type::Null; }
    bool is_boolean() const { return type_ == Type::Boolean; }
    bool is_byte() const { return type_ == Type::Byte; }
    bool is_short() const { return type_ == Type::Short; }
    bool is_integer() const { return type_ == Type::Integer; }
    bool is_long() const { return type_ == Type::Long; }
    bool is_real() const { return type_ == Type::Real; }
    bool is_float() const { return type_ == Type::Float; }
    bool is_char() const { return type_ == Type::Char; }
    bool is_symbol() const { return type_ == Type::Symbol; }
    bool is_date() const { return type_ == Type::Date; }
    bool is_month() const { return type_ == Type::Month; }
    bool is_time() const { return type_ == Type::Time; }
    bool is_minute() const { return type_ == Type::Minute; }
    bool is_second() const { return type_ == Type::Second; }
    bool is_datetime() const { return type_ == Type::DateTime; }
    bool is_timespan() const { return type_ == Type::TimeSpan; }

    // Value getters with type checking
    bool get_boolean() const {
        if (!is_boolean()) throw std::runtime_error("Not a boolean value");
        return bool_val_;
    }
    
    unsigned char get_byte() const {
        if (!is_byte()) throw std::runtime_error("Not a byte value");
        return byte_val_;
    }
    
    char get_char() const {
        if (!is_char()) throw std::runtime_error("Not a char value");
        return char_val_;
    }
    
    short get_short() const {
        if (!is_short()) throw std::runtime_error("Not a short value");
        return short_val_;
    }
    
    int get_integer() const {
        if (!is_integer()) throw std::runtime_error("Not an integer value");
        return int_val_;
    }
    
    long long get_long() const {
        if (!is_long()) throw std::runtime_error("Not a long value");
        return long_val_;
    }
    
    float get_real() const {
        if (!is_real()) throw std::runtime_error("Not a real value");
        return real_val_;
    }
    
    double get_float() const {
        if (!is_float()) throw std::runtime_error("Not a float value");
        return float_val_;
    }
    
    const std::string& get_symbol() const {
        if (!is_symbol()) throw std::runtime_error("Not a symbol value");
        return string_val_;
    }

    // Temporal type getters
    long long get_date() const {
        if (!is_date()) throw std::runtime_error("Not a date value");
        return long_val_;
    }

    int get_month() const {
        if (!is_month()) throw std::runtime_error("Not a month value");
        return int_val_;
    }

    int get_time() const {
        if (!is_time()) throw std::runtime_error("Not a time value");
        return int_val_;
    }

    int get_minute() const {
        if (!is_minute()) throw std::runtime_error("Not a minute value");
        return int_val_;
    }

    int get_second() const {
        if (!is_second()) throw std::runtime_error("Not a second value");
        return int_val_;
    }

    double get_datetime() const {
        if (!is_datetime()) throw std::runtime_error("Not a datetime value");
        return float_val_;
    }

    long long get_timespan() const {
        if (!is_timespan()) throw std::runtime_error("Not a timespan value");
        return long_val_;
    }

    // Generic converter to string for display
    std::string to_string() const {
        if (is_null()) return "null";
        
        switch (type_) {
            case Type::Boolean: return bool_val_ ? "true" : "false";
            case Type::Byte: return std::to_string(static_cast<int>(byte_val_));
            case Type::Char: return std::string(1, char_val_);
            case Type::Short: return std::to_string(short_val_);
            case Type::Integer: return std::to_string(int_val_);
            case Type::Long: return std::to_string(long_val_);
            case Type::Real: {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(7) << real_val_;
                return oss.str();
            }
            case Type::Float: {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(7) << float_val_;
                return oss.str();
            }
            case Type::Symbol: return string_val_;
            case Type::Date: return format_date(long_val_);
            case Type::Month: return format_month(int_val_);
            case Type::Time: return format_time(int_val_);
            case Type::Minute: return format_minute(int_val_);
            case Type::Second: return format_second(int_val_);
            case Type::DateTime: return format_datetime(float_val_);
            case Type::TimeSpan: return format_timespan(long_val_);
            default: return "unknown";
        }
    }

    /**
     * @brief Retrieves the type of the stored value.
     *
     * @return Type The type of the value as defined in the `Type` enum.
     */
    Type get_type() const { return type_; }

private:
    Type type_;
    union {
        bool bool_val_;
        unsigned char byte_val_;
        char char_val_;
        short short_val_;
        int int_val_;
        long long long_val_;
        float real_val_;
        double float_val_;
    };
    std::string string_val_;  // Outside union due to non-trivial destructor

    // Helper functions for formatting temporal types
    static std::string format_date(long long days) {
        std::time_t time = (days * 86400LL) + 946684800LL;  // Adjust for KDB+ epoch (2000.01.01)
        std::tm* tm = std::gmtime(&time);
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d");
        return oss.str();
    }

    static std::string format_month(int months) {
        int year = 2000 + (months / 12);  // Base year 2000 for KDB+
        int month = (months % 12) + 1;
        std::ostringstream oss;
        oss << year << '.' << std::setfill('0') << std::setw(2) << month;
        return oss.str();
    }

    static std::string format_time(int milliseconds) {
        int hours = milliseconds / (3600 * 1000);
        int minutes = (milliseconds % (3600 * 1000)) / (60 * 1000);
        int seconds = (milliseconds % (60 * 1000)) / 1000;
        int ms = milliseconds % 1000;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours << ':'
            << std::setfill('0') << std::setw(2) << minutes << ':'
            << std::setfill('0') << std::setw(2) << seconds << '.'
            << std::setfill('0') << std::setw(3) << ms;
        return oss.str();
    }

    static std::string format_minute(int minutes) {
        int hours = minutes / 60;
        int mins = minutes % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours << ':'
            << std::setfill('0') << std::setw(2) << mins;
        return oss.str();
    }

    static std::string format_second(int seconds) {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        int secs = seconds % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours << ':'
            << std::setfill('0') << std::setw(2) << minutes << ':'
            << std::setfill('0') << std::setw(2) << secs;
        return oss.str();
    }

    static std::string format_datetime(double days) {
        std::time_t time = static_cast<std::time_t>(days * 86400.0) + 946684800LL;
        std::tm* tm = std::gmtime(&time);
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    static std::string format_timespan(long long nanoseconds) {
        long long days = nanoseconds / (86400LL * 1000000000LL);
        nanoseconds %= (86400LL * 1000000000LL);
        long long hours = nanoseconds / (3600LL * 1000000000LL);
        nanoseconds %= (3600LL * 1000000000LL);
        long long minutes = nanoseconds / (60LL * 1000000000LL);
        nanoseconds %= (60LL * 1000000000LL);
        long long seconds = nanoseconds / 1000000000LL;
        nanoseconds %= 1000000000LL;

        std::ostringstream oss;
        oss << days << "D"
            << std::setfill('0') << std::setw(2) << hours << ':'
            << std::setfill('0') << std::setw(2) << minutes << ':'
            << std::setfill('0') << std::setw(2) << seconds << '.'
            << std::setfill('0') << std::setw(9) << nanoseconds;
        return oss.str();
    }
};

using KDBRow = std::vector<KDBValue>;
using KDBTable = std::vector<KDBRow>;

/**
 * @brief Represents the result of a KDB+ query.
 *
 * The `KDBResult` class can hold a single value, a single row, or an entire table.
 * It provides type-safe access to the stored data and utility functions for interacting with it.
 */
class KDBResult {
public:
    /**
     * @enum Type
     * @brief Enumerates the possible types of results.
     */
    enum class Type {
        Value,
        Row,
        Table
    };

    // Constructors
    explicit KDBResult(KDBValue value) : type_(Type::Value), value_(std::move(value)) {}
    explicit KDBResult(KDBRow row) : type_(Type::Row), row_(std::move(row)) {}
    explicit KDBResult(KDBTable table) : type_(Type::Table), table_(std::move(table)) {}

    // Getter methods for the stored result
    /**
     * @brief Retrieves the stored value.
     *
     * @return const KDBValue& The stored value.
     * @throws std::runtime_error If the result is not a single value.
     */
    const KDBValue& get_value() const {
        if (type_ != Type::Value)
            throw std::runtime_error("Result is not a single value");
        return value_;
    }

    /**
     * @brief Retrieves the stored row.
     *
     * @return const KDBRow& The stored row.
     * @throws std::runtime_error If the result is not a row.
     */
    const KDBRow& get_row() const {
        if (type_ != Type::Row)
            throw std::runtime_error("Result is not a row");
        return row_;
    }

    /**
     * @brief Retrieves the stored table.
     *
     * @return const KDBTable& The stored table.
     * @throws std::runtime_error If the result is not a table.
     */
    const KDBTable& get_table() const {
        if (type_ != Type::Table)
            throw std::runtime_error("Result is not a table");
        return table_;
    }

    /**
     * @brief Retrieves the type of the result.
     *
     * @return Type The type of the result.
     */
    Type get_type() const { return type_; }

    /**
     * @brief Retrieves the type of the result as a string.
     *
     * @return std::string The type of the result ("Value", "Row", "Table", or "Unknown").
     */
    std::string get_type_string() const {
        switch (type_) {
            case Type::Value: return "Value";
            case Type::Row: return "Row";
            case Type::Table: return "Table";
            default: return "Unknown";
        }
    }

    // Convenience methods to check type
    bool is_value() const { return type_ == Type::Value; }
    bool is_row() const { return type_ == Type::Row; }
    bool is_table() const { return type_ == Type::Table; }

    // Size information based on type
    /**
     * @brief Retrieves the size of the result.
     *
     * - For `Value`: returns 1.
     * - For `Row`: returns the number of columns.
     * - For `Table`: returns the number of rows.
     *
     * @return size_t The size based on the result type.
     */
    size_t size() const {
        switch (type_) {
            case Type::Value: return 1;
            case Type::Row: return row_.size();
            case Type::Table: return table_.size();
            default: return 0;
        }
    }

    // Print function remains removed

private:
    Type type_;
    KDBValue value_;
    KDBRow row_;
    KDBTable table_;
};

/**
 * @struct ColumnMeta
 * @brief Stores metadata information for a table column.
 *
 * Contains the column name and its corresponding type code.
 */
struct ColumnMeta {
    std::string name;   ///< The name of the column.
    int type_code;      ///< The type code of the column.
};

// Function declarations
std::vector<ColumnMeta> get_metadata(const std::string& table_name, bool internal_use = false);
KDBResult iloc(const std::string& table_name, const std::vector<int>& rows, const std::vector<int>& cols);
KDBResult loc(const std::string& table_name, const std::string& condition);

#endif
