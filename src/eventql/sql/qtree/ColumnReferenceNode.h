/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

using namespace stx;

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
      stx::OutputStream* os);

  static RefPtr<QueryTreeNode> decode (
      QueryTreeCoder* coder,
      stx::InputStream* is);

protected:
  String column_name_;
  Option<size_t> column_index_;
};

} // namespace csql
