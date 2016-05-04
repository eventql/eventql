/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>

namespace csql {

struct ColumnInfo {
  String column_name;
  String type;
  bool is_nullable;
  size_t type_size;
};

struct TableInfo {
  String table_name;
  Option<String> description;
  Vector<ColumnInfo> columns;
  Set<String> tags;
};

} // namespace csql
