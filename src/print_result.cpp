#include "print_result.h"
#include <ctime>
#include <chrono>
namespace {
    // Helper function implementations
    void print_separator(const std::vector<size_t>& widths, const std::string& indent) {
        std::cout << indent << "+";
        for (size_t w : widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << std::endl;
    }

    std::string get_k_type_name(int type) {
        switch (type) {
            case -KB: return "Boolean";
            case -KG: return "Byte";
            case -KH: return "Short";
            case -KI: return "Int";
            case -KJ: return "Long";
            case -KE: return "Real";
            case -KF: return "Float";
            case -KC: return "Char";
            case -KS: return "Symbol";
            case -KP: return "Timestamp";
            case -KM: return "Month";
            case -KD: return "Date";
            case -KZ: return "DateTime";
            case -KU: return "Minute";
            case -KV: return "Second";
            case -KT: return "Time";
            case KB: return "Boolean List";
            case KG: return "Byte List";
            case KH: return "Short List";
            case KI: return "Int List";
            case KJ: return "Long List";
            case KE: return "Real List";
            case KF: return "Float List";
            case KC: return "Char List";
            case KS: return "Symbol List";
            case KP: return "Timestamp List";
            case KM: return "Month List";
            case KD: return "Date List";
            case KZ: return "DateTime List";
            case KU: return "Minute List";
            case KV: return "Second List";
            case KT: return "Time List";
            case XT: return "Table";
            case XD: return "Keyed Table";
            default: return "Unknown";
        }
    }

    std::string format_k_value(K obj, J idx) {
        if (!obj) return "null";
        
        std::ostringstream ss;
        switch (obj->t) {
                // Time atom (-KT)
                case -KT: {
                    int millis = obj->i;
                    if (millis == ni) {
                        ss << "0Nt";
                    } else {
                        int hours = millis / 3600000;
                        int minutes = (millis % 3600000) / 60000;
                        int seconds = (millis % 60000) / 1000;
                        int ms = millis % 1000;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds << "."
                           << std::setfill('0') << std::setw(3) << ms;
                    }
                    break;
                }
                
                // Time vector (KT)
                case KT: {
                    int millis = kI(obj)[idx];
                    if (millis == ni) {
                        ss << "0Nt";
                    } else {
                        int hours = millis / 3600000;
                        int minutes = (millis % 3600000) / 60000;
                        int seconds = (millis % 60000) / 1000;
                        int ms = millis % 1000;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds << "."
                           << std::setfill('0') << std::setw(3) << ms;
                    }
                    break;
                }

                // Timestamp atom (-KP)
                case -KP: {
                    J nanos = obj->j;
                    if (nanos == nj) {
                        ss << "0Np";
                    } else {
                        // Convert nanoseconds since 2000.01.01 to total nanoseconds since epoch
                        nanos += 946684800000000000LL;  // Offset for 2000.01.01
                        
                        // Extract seconds and remaining nanoseconds
                        J seconds = nanos / 1000000000LL;
                        J remaining_nanos = nanos % 1000000000LL;
                        
                        // Convert to time structure
                        time_t time_seconds = static_cast<time_t>(seconds);
                        struct tm *timeinfo = localtime(&time_seconds);
                        
                        // Format the timestamp
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%dD%H:%M:%S", timeinfo);
                        ss << buffer;
                        
                        // Add nanoseconds if non-zero
                        if (remaining_nanos > 0) {
                            ss << "." << std::setfill('0') << std::setw(9) << remaining_nanos;
                        }
                    }
                    break;
                }

                // Timestamp vector (KP)
                case KP: {
                    J nanos = kJ(obj)[idx];
                    if (nanos == nj) {
                        ss << "0Np";
                    } else {
                        // Convert nanoseconds since 2000.01.01 to total nanoseconds since epoch
                        nanos += 946684800000000000LL;  // Offset for 2000.01.01
                        
                        // Extract seconds and remaining nanoseconds
                        J seconds = nanos / 1000000000LL;
                        J remaining_nanos = nanos % 1000000000LL;
                        
                        // Convert to time structure
                        time_t time_seconds = static_cast<time_t>(seconds);
                        struct tm *timeinfo = localtime(&time_seconds);
                        
                        // Format the timestamp
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%dD%H:%M:%S", timeinfo);
                        ss << buffer;
                        
                        // Add nanoseconds if non-zero
                        if (remaining_nanos > 0) {
                            ss << "." << std::setfill('0') << std::setw(9) << remaining_nanos;
                        }
                    }
                    break;
                }
                // DateTime atom (-KZ)
                case -KZ: {
                    double days = obj->f;
                    if (std::isnan(days)) {
                        ss << "0Nz";
                    } else {
                        time_t seconds = static_cast<time_t>((days + 10957) * 86400);  // Days since 1970
                        struct tm *timeinfo = localtime(&seconds);
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%d %H:%M:%S", timeinfo);
                        ss << buffer << "." << std::setfill('0') << std::setw(3)
                           << static_cast<int>((days - std::floor(days)) * 86400000) % 1000;
                    }
                    break;
                }

                // DateTime vector (KZ)
                case KZ: {
                    double days = kF(obj)[idx];
                    if (std::isnan(days)) {
                        ss << "0Nz";
                    } else {
                        time_t seconds = static_cast<time_t>((days + 10957) * 86400);
                        struct tm *timeinfo = localtime(&seconds);
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%d %H:%M:%S", timeinfo);
                        ss << buffer << "." << std::setfill('0') << std::setw(3)
                           << static_cast<int>((days - std::floor(days)) * 86400000) % 1000;
                    }
                    break;
                }

                // Timespan atom (-KN)
                case -KN: {
                    J span = obj->j;
                    if (span == nj) {
                        ss << "0Nn";
                    } else {
                        J nanos = std::abs(span);
                        bool negative = span < 0;
                        J days = nanos / (86400000000000LL);
                        nanos %= 86400000000000LL;
                        J hours = nanos / 3600000000000LL;
                        nanos %= 3600000000000LL;
                        J minutes = nanos / 60000000000LL;
                        nanos %= 60000000000LL;
                        J seconds = nanos / 1000000000LL;
                        nanos %= 1000000000LL;

                        if (negative) ss << "-";
                        if (days > 0) ss << days << "D";
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds;
                        if (nanos > 0) {
                            ss << "." << std::setfill('0') << std::setw(9) << nanos;
                        }
                    }
                    break;
                }

                // Timespan vector (KN)
                case KN: {
                    J span = kJ(obj)[idx];
                    if (span == nj) {
                        ss << "0Nn";
                    } else {
                        J nanos = std::abs(span);
                        bool negative = span < 0;
                        J days = nanos / (86400000000000LL);
                        nanos %= 86400000000000LL;
                        J hours = nanos / 3600000000000LL;
                        nanos %= 3600000000000LL;
                        J minutes = nanos / 60000000000LL;
                        nanos %= 60000000000LL;
                        J seconds = nanos / 1000000000LL;
                        nanos %= 1000000000LL;

                        if (negative) ss << "-";
                        if (days > 0) ss << days << "D";
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds;
                        if (nanos > 0) {
                            ss << "." << std::setfill('0') << std::setw(9) << nanos;
                        }
                    }
                    break;
                }

                // Date atom (-KD)
                case -KD: {
                    if (obj->i == ni) {
                        ss << "0Nd";
                    } else {
                        time_t seconds = static_cast<time_t>(obj->i * 86400);  // Days since 2000.01.01
                        seconds += 946684800;  // Add seconds from 1970 to 2000
                        struct tm *timeinfo = localtime(&seconds);
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%d", timeinfo);
                        ss << buffer;
                    }
                    break;
                }

                // Date vector (KD)
                case KD: {
                    if (kI(obj)[idx] == ni) {
                        ss << "0Nd";
                    } else {
                        time_t seconds = static_cast<time_t>(kI(obj)[idx] * 86400);
                        seconds += 946684800;
                        struct tm *timeinfo = localtime(&seconds);
                        char buffer[80];
                        strftime(buffer, sizeof(buffer), "%Y.%m.%d", timeinfo);
                        ss << buffer;
                    }
                    break;
                }
                
                // Month atom (-KM)
                case -KM: {
                    if (obj->i == ni) {
                        ss << "0Nm";
                    } else {
                        int year = 2000 + (obj->i / 12);
                        int month = (obj->i % 12) + 1;
                        ss << year << "." << std::setfill('0') << std::setw(2) << month;
                    }
                    break;
                }

                // Month vector (KM)
                case KM: {
                    if (kI(obj)[idx] == ni) {
                        ss << "0Nm";
                    } else {
                        int year = 2000 + (kI(obj)[idx] / 12);
                        int month = (kI(obj)[idx] % 12) + 1;
                        ss << year << "." << std::setfill('0') << std::setw(2) << month;
                    }
                    break;
                }

                // Minute atom (-KU)
                case -KU: {
                    if (obj->i == ni) {
                        ss << "0Nu";
                    } else {
                        int hours = obj->i / 60;
                        int minutes = obj->i % 60;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes;
                    }
                    break;
                }

                // Minute vector (KU)
                case KU: {
                    if (kI(obj)[idx] == ni) {
                        ss << "0Nu";
                    } else {
                        int hours = kI(obj)[idx] / 60;
                        int minutes = kI(obj)[idx] % 60;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes;
                    }
                    break;
                }

                // Second atom (-KV)
                case -KV: {
                    if (obj->i == ni) {
                        ss << "0Nv";
                    } else {
                        int hours = obj->i / 3600;
                        int minutes = (obj->i % 3600) / 60;
                        int seconds = obj->i % 60;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds;
                    }
                    break;
                }

                // Second vector (KV)
                case KV: {
                    if (kI(obj)[idx] == ni) {
                        ss << "0Nv";
                    } else {
                        int hours = kI(obj)[idx] / 3600;
                        int minutes = (kI(obj)[idx] % 3600) / 60;
                        int seconds = kI(obj)[idx] % 60;
                        ss << std::setfill('0') << std::setw(2) << hours << ":"
                           << std::setfill('0') << std::setw(2) << minutes << ":"
                           << std::setfill('0') << std::setw(2) << seconds;
                    }
                    break;
                }
            
            // Standard types
            case -KB: ss << (obj->g ? "true" : "false"); break;
            case -KG: ss << static_cast<int>(obj->g); break;
            case -KH: ss << obj->h; break;
            case -KI: ss << obj->i; break;
            case -KJ: ss << obj->j; break;
            case -KE: ss.precision(7); ss << std::fixed << obj->e; break;
            case -KF: ss.precision(7); ss << std::fixed << obj->f; break;
            case -KC: ss << "'" << obj->g << "'"; break;
            case -KS: ss << (obj->s ? ("`" + std::string(obj->s)) : "0N"); break;
            
            case KB: ss << (kG(obj)[idx] == 0 ? "0N" : (kG(obj)[idx] > 0 ? "true" : "false")); break;
            case KG: ss << (kG(obj)[idx] == 0 ? "0N" : std::to_string(static_cast<int>(kG(obj)[idx]))); break;
            case KH: ss << (kH(obj)[idx] == nh ? "0N" : std::to_string(kH(obj)[idx])); break;
            case KI: ss << (kI(obj)[idx] == ni ? "0N" : std::to_string(kI(obj)[idx])); break;
            case KJ: ss << (kJ(obj)[idx] == nj ? "0N" : std::to_string(kJ(obj)[idx])); break;
            case KE: ss.precision(7); ss << std::fixed << (std::isnan(kE(obj)[idx]) ? "0N" : std::to_string(kE(obj)[idx])); break;
            case KF: ss.precision(7); ss << std::fixed << (std::isnan(kF(obj)[idx]) ? "0N" : std::to_string(kF(obj)[idx])); break;
            case KC: ss << (kC(obj)[idx] == ' ' ? "0N" : ("'" + std::string(1, kC(obj)[idx]) + "'")); break;
            case KS: ss << (kS(obj)[idx] == NULL ? "0N" : ("`" + std::string(kS(obj)[idx]))); break;
            
            default: ss << "?"; break;
        }
        return ss.str();
    }

    void print_k_table(K obj, const std::string& indent) {
        K dict = (obj->t == XD) ? kK(obj)[1]->k : obj->k;
        K key_dict = (obj->t == XD) ? kK(obj)[0]->k : nullptr;
        
        if (!dict) {
            std::cout << indent << "Invalid table structure" << std::endl;
            return;
        }

        K names = kK(dict)[0];
        K values = kK(dict)[1];
        K key_names = key_dict ? kK(key_dict)[0] : nullptr;
        K key_values = key_dict ? kK(key_dict)[1] : nullptr;
        
        std::vector<size_t> widths;
        
        if (key_dict) {
            for (J i = 0; i < key_names->n; i++) {
                size_t width = strlen(kS(key_names)[i]);
                K coldata = kK(key_values)[i];
                for (J row = 0; row < coldata->n; row++) {
                    width = std::max(width, format_k_value(coldata, row).length());
                }
                widths.push_back(width);
            }
        }
        
        for (J i = 0; i < names->n; i++) {
            size_t width = strlen(kS(names)[i]);
            K coldata = kK(values)[i];
            for (J row = 0; row < coldata->n; row++) {
                width = std::max(width, format_k_value(coldata, row).length());
            }
            widths.push_back(width);
        }

        std::cout << indent << "Type: " << get_k_type_name(obj->t) << std::endl;
        
        print_separator(widths, indent);
        
        std::cout << indent << "|";
        if (key_dict) {
            for (J i = 0; i < key_names->n; i++) {
                std::cout << " " << std::setw(widths[i]) << std::left << kS(key_names)[i] << " |";
            }
        }
        for (J i = 0; i < names->n; i++) {
            size_t width_idx = i + (key_names ? key_names->n : 0);
            std::cout << " " << std::setw(widths[width_idx]) << std::left << kS(names)[i] << " |";
        }
        std::cout << std::endl;
        
        print_separator(widths, indent);
        
        J rows = kK(values)[0]->n;
        for (J row = 0; row < rows; row++) {
            std::cout << indent << "|";
            if (key_dict) {
                for (J col = 0; col < key_values->n; col++) {
                    K coldata = kK(key_values)[col];
                    std::cout << " " << std::setw(widths[col]) << std::left
                             << format_k_value(coldata, row) << " |";
                }
            }
            for (J col = 0; col < values->n; col++) {
                K coldata = kK(values)[col];
                size_t width_idx = col + (key_names ? key_names->n : 0);
                std::cout << " " << std::setw(widths[width_idx]) << std::left
                         << format_k_value(coldata, row) << " |";
            }
            std::cout << std::endl;
        }
        
        print_separator(widths, indent);
        std::cout << indent << "Total rows: " << rows << std::endl;
    }
} // anonymous namespace

void print_result_impl(const std::variant<K, KDBResult>& result,
                      const std::vector<ColumnMeta>& metadata,
                      int indent) {
    std::string indent_str(indent, ' ');

    if (std::holds_alternative<K>(result)) {
        K obj = std::get<K>(result);
        if (!obj) {
            std::cout << indent_str << "null" << std::endl;
            return;
        }

        if (obj->t == -128) {
            std::cout << indent_str << "ERROR: " << obj->s << std::endl;
            return;
        }

        if (obj->t < 0) {
            std::cout << indent_str << "Type " << obj->t << " (" << get_k_type_name(obj->t) << "): " << format_k_value(obj) << std::endl;
        }
        else if (obj->t == XT || obj->t == XD) {
            print_k_table(obj, indent_str);
        }
        else if (obj->t > 0 && obj->t < 20) {
            std::cout << indent_str << "Type " << obj->t << " (" << get_k_type_name(obj->t)
                     << ") [" << obj->n << "]: [";
            for (J i = 0; i < std::min(obj->n, (J)10); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << format_k_value(obj, i);
            }
            if (obj->n > 10) std::cout << ", ...";
            std::cout << "]" << std::endl;
        }
        else if (obj->t == 0) {
            std::cout << indent_str << "Generic List [" << obj->n << "]:" << std::endl;
            for (J i = 0; i < std::min(obj->n, (J)5); ++i) {
                std::cout << indent_str << "[" << i << "] ";
                print_result_impl(std::variant<K, KDBResult>(kK(obj)[i]), metadata, indent + 2);
            }
            if (obj->n > 5) {
                std::cout << indent_str << "..." << std::endl;
            }
        }
        else {
            std::cout << indent_str << "Unhandled K type " << obj->t << std::endl;
        }
    }
    else {  // KDBResult
        const KDBResult& kdb_result = std::get<KDBResult>(result);
        
        switch (kdb_result.get_type()) {
            case KDBResult::Type::Value: {
                std::cout << indent_str << "KDB Value: "
                         << kdb_result.get_value().to_string() << std::endl;
                break;
            }
            case KDBResult::Type::Row: {
                const auto& row = kdb_result.get_row();
                std::cout << indent_str << "KDB Row:" << std::endl;
                
                std::vector<size_t> widths;
                for (size_t i = 0; i < row.size(); ++i) {
                    // Just use the data width if no metadata
                    size_t width = row[i].to_string().length();
                    widths.push_back(width);
                }
                
                print_separator(widths, indent_str);
                
                // Only print header if metadata is available and non-empty
                if (!metadata.empty()) {
                    std::cout << indent_str << "|";
                    for (size_t i = 0; i < metadata.size() && i < row.size(); ++i) {
                        std::cout << " " << std::setw(widths[i]) << std::left
                                 << metadata[i].name << " |";
                    }
                    std::cout << std::endl;
                    print_separator(widths, indent_str);
                }
                
                // Row data
                std::cout << indent_str << "|";
                for (size_t i = 0; i < row.size(); ++i) {
                    std::cout << " " << std::setw(widths[i]) << std::left
                             << row[i].to_string() << " |";
                }
                std::cout << std::endl;
                print_separator(widths, indent_str);
                break;
            }

            case KDBResult::Type::Table: {
                const auto& table = kdb_result.get_table();
                if (table.empty()) {
                    std::cout << indent_str << "Empty KDB Table" << std::endl;
                    return;
                }

                std::cout << indent_str << "KDB Table:" << std::endl;
                
                // Determine number of columns from data
                size_t num_columns = 0;
                for (const auto& row : table) {
                    num_columns = std::max(num_columns, row.size());
                }

                // Initialize widths based on data only
                std::vector<size_t> widths(num_columns, 0);

                // Calculate widths from data
                for (const auto& row : table) {
                    for (size_t col = 0; col < row.size(); ++col) {
                        widths[col] = std::max(widths[col], row[col].to_string().length());
                    }
                }

                print_separator(widths, indent_str);
                
                // Only print header if metadata is available and non-empty
                if (!metadata.empty()) {
                    std::cout << indent_str << "|";
                    for (size_t i = 0; i < metadata.size() && i < num_columns; ++i) {
                        // Update width if column header is wider than data
                        widths[i] = std::max(widths[i], metadata[i].name.length());
                        std::cout << " " << std::setw(widths[i]) << std::left
                                 << metadata[i].name << " |";
                    }
                    std::cout << std::endl;
                    print_separator(widths, indent_str);
                }
                
                // Rows
                for (const auto& row : table) {
                    std::cout << indent_str << "|";
                    for (size_t i = 0; i < row.size(); ++i) {
                        std::cout << " " << std::setw(widths[i]) << std::left
                                 << row[i].to_string() << " |";
                    }
                    std::cout << std::endl;
                }
                print_separator(widths, indent_str);
                std::cout << indent_str << "Total rows: " << table.size() << std::endl;
                break;
            }
        }
    }
}
