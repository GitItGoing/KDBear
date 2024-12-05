#ifndef READ_CSV_H
#define READ_CSV_H

#include <string>
#include <vector>

bool read_csv(const std::string& table_name,
              const std::string& filename,
              bool header = true,
              char delimiter = ',',
              const std::string& key_column = "",
              const std::vector<std::string>& column_types = {});

#endif // READ_CSV_H
