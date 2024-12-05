#ifndef PRINT_RESULT_H
#define PRINT_RESULT_H

#include "k.h"
#include "select_from_table.h"
#include <variant>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <sstream>

// Forward declarations
namespace {
    void print_separator(const std::vector<size_t>& widths, const std::string& indent = "");
    std::string get_k_type_name(int type);
    std::string format_k_value(K obj, J idx = 0);
    void print_k_table(K obj, const std::string& indent);
}

// Main variant-based print_result function declaration
void print_result_impl(const std::variant<K, KDBResult>& result,
                      const std::vector<ColumnMeta>& metadata,
                      int indent);

// Public interface functions
inline void print_result(const std::variant<K, KDBResult>& result,
                        const std::vector<ColumnMeta>& metadata = std::vector<ColumnMeta>(),
                        int indent = 0) {
    print_result_impl(result, metadata, indent);
}

inline void print_result(const KDBResult& result,
                        const std::vector<ColumnMeta>& metadata = std::vector<ColumnMeta>(),
                        int indent = 0) {
    print_result_impl(std::variant<K, KDBResult>(result), metadata, indent);
}

inline void print_result(K result,
                        const std::vector<ColumnMeta>& metadata = std::vector<ColumnMeta>(),
                        int indent = 0) {
    print_result_impl(std::variant<K, KDBResult>(result), metadata, indent);
}

inline void print_result(const KDBTable& table,
                        const std::vector<ColumnMeta>& metadata = std::vector<ColumnMeta>(),
                        int indent = 0) {
    print_result_impl(KDBResult(table), metadata, indent);
}

#endif // PRINT_RESULT_H
