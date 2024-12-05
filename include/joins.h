// joins.h
#ifndef JOINS_H
#define JOINS_H

#include "k.h"
#include "connections.h"
#include <string>
#include <vector>

namespace joins {
    // Core join functions
    K inner_join(const std::string& table1,
                 const std::string& table2,
                 const std::string& result_name,
                 const std::vector<std::string>& join_columns = std::vector<std::string>());

    K left_join(const std::string& table1,
                const std::string& table2,
                const std::string& result_name,
                const std::vector<std::string>& join_columns = std::vector<std::string>());

    K right_join(const std::string& table1,
                 const std::string& table2,
                 const std::string& result_name,
                 const std::vector<std::string>& join_columns = std::vector<std::string>());

    // Window Join
    K window_join(const std::string& table1,
                  const std::string& table2,
                  const std::string& result_name,
                  const std::string& time_column_left,
                  const std::string& time_column_right,
                  double window_size_seconds,
                  const std::vector<std::string>& join_columns);

    K asof_join(const std::string& table1,
                const std::string& table2,
                const std::string& result_name,
                const std::string& time_column_left,
                const std::string& time_column_right,
                const std::vector<std::string>& join_columns);

    K union_join(const std::string& table1,
                const std::string& table2,
                const std::string& result_name,
                const std::vector<std::string>& join_columns = std::vector<std::string>());

    // Helper functions
    namespace detail {
        bool prepare_tables(const std::string& table1,
                          const std::string& table2,
                          std::string& t1_unkeyed,
                          std::string& t2_unkeyed);
        void cleanup_tables(const std::string& t1_unkeyed, const std::string& t2_unkeyed);
        std::string build_join_by(const std::vector<std::string>& join_columns);
        K execute_join(const std::string& query,
                      const std::string& result_name,
                      const std::string& t1_unkeyed,
                      const std::string& t2_unkeyed);
    }
}

#endif
