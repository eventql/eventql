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
#include <fnord/stdtypes.h>
#include <chartsql/qtree/TableExpressionNode.h>

using namespace fnord;

namespace csql {

class LimitNode : public TableExpressionNode {
public:

  LimitNode(
      size_t limit,
      size_t offset,
      RefPtr<TableExpressionNode> table);

  RefPtr<TableExpressionNode> inputTable() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

protected:
  size_t limit_;
  size_t offset_;
  RefPtr<TableExpressionNode> table_;
};

} // namespace csql
