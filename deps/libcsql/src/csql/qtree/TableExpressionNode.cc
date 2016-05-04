/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/qtree/TableExpressionNode.h>
#include <stx/inspect.h>

using namespace stx;

namespace csql {

size_t TableExpressionNode::numColumns() const {
  return outputColumns().size();
}

//size_t TableExpressionNode::getColumnIndex(const String& column_name) const {
//  {
//    auto iter = internal_columns_.find(column_name);
//    if (iter != internal_columns_.end()) {
//      return iter->second;
//    }
//  }
//
//  auto cols = columnNames();
//
//  for (int i = 0; i < cols.size(); ++i) {
//    if (cols[i] == column_name) {
//      return i;
//    }
//  }
//
//  {
//    auto internal_idx = addInternalColumn(column_name);
//    if (internal_idx != size_t(-1)) {
//      internal_columns_.emplace(column_name, internal_idx);
//      return internal_idx;
//    }
//  }
//
//
//  return -1;
//}

} // namespace csql
