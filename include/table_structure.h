// table_structure.h
#ifndef TABLE_STRUCTURE_H
#define TABLE_STRUCTURE_H

#include "k.h"
#include "connections.h"
#include <tuple>

std::tuple<int, int> shape(const std::variant<K, std::string>& table_input);

#endif
