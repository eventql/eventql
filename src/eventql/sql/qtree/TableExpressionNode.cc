/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/util/inspect.h>

using namespace util;

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
