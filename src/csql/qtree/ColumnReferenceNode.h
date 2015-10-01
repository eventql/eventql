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
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <csql/qtree/ValueExpressionNode.h>

using namespace stx;

namespace csql {

class ColumnReferenceNode : public ValueExpressionNode {
public:

  explicit ColumnReferenceNode(const ColumnReferenceNode& other);
  explicit ColumnReferenceNode(const String& column_name);
  explicit ColumnReferenceNode(size_t column_index_);

  const String& fieldName() const;

  size_t columnIndex() const;

  void setColumnIndex(size_t index);

  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toSQL() const override;

protected:
  String column_name_;
  Option<size_t> column_index_;
};

} // namespace csql
