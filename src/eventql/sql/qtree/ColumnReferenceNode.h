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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

using namespace util;

namespace csql {

class ColumnReferenceNode : public ValueExpressionNode {
public:

  explicit ColumnReferenceNode(const ColumnReferenceNode& other);
  explicit ColumnReferenceNode(const String& column_name);
  explicit ColumnReferenceNode(size_t column_index_);

  const String& fieldName() const; // DEPRECATED

  const String& columnName() const;
  void setColumnName(const String& name);

  size_t columnIndex() const;

  void setColumnIndex(size_t index);
  bool hasColumnIndex() const;

  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toSQL() const override;

  static void encode(
      QueryTreeCoder* coder,
      const ColumnReferenceNode& node,
      util::OutputStream* os);

  static RefPtr<QueryTreeNode> decode (
      QueryTreeCoder* coder,
      util::InputStream* is);

protected:
  String column_name_;
  Option<size_t> column_index_;
};

} // namespace csql
