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
#include <eventql/util/io/outputstream.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/qtree_coder.h>

using namespace stx;

namespace csql {

class LimitNode : public TableExpressionNode {
public:

  LimitNode(
      size_t limit,
      size_t offset,
      RefPtr<QueryTreeNode> table);

  RefPtr<QueryTreeNode> inputTable() const;

  Vector<String> outputColumns() const override;

  Vector<QualifiedColumn> allColumns() const override;

  size_t getColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  size_t limit() const;

  size_t offset() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  static void encode(
      QueryTreeCoder* coder,
      const LimitNode& node,
      stx::OutputStream* os);

  static RefPtr<QueryTreeNode> decode(
      QueryTreeCoder* coder,
      stx::InputStream* os);

protected:
  size_t limit_;
  size_t offset_;
  RefPtr<QueryTreeNode> table_;
};

} // namespace csql
