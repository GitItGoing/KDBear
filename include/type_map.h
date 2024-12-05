#ifndef TYPE_MAP_H
#define TYPE_MAP_H

#include "k.h"
#include "connections.h"
#include <string>
#include <unordered_map>
#include <vector>

// TypeInfo structure definition
struct TypeInfo {
    int kdb_type;           // KDB+ type code
    std::string name;       // Type name
    char type_char;         // Single char type identifier
    bool (*validator)(const std::string&);  // Type validation function
    void (*null_assigner)(K, size_t);      // Null value assignment function
    void (*value_assigner)(K, const std::string&, size_t);  // Value assignment function
    std::string (*formatter)(K, size_t);    // Value formatting function
    std::string null_initializer;  // KDB+ string for initializing null/empty value
};

// Main type map interfaces
const std::unordered_map<std::string, TypeInfo>& getExtendedTypeMap();
std::unordered_map<std::string, std::pair<int, std::string>> getTypeMap();

namespace detail {
    // Parsing functions
    I parse_date(const std::string& s);
    F parse_datetime(const std::string& s);
    I parse_time(const std::string& s);
}

// Value handling functions
bool is_null_value(K col_data, size_t idx);
void assign_null_value(K col_data, size_t idx);
void assign_value(K col_data, const std::string& value, size_t idx);
std::string format_value(K col_data, size_t idx);

// Type inference
I infer_column_type(const std::vector<std::string>& data);

#endif // TYPE_MAP_H
