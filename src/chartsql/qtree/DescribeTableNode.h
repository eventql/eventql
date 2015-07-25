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
#include <chartsql/qtree/TableExpressionNode.h>

using namespace stx;

namespace csql {

class DescribeTableNode : public TableExpressionNode {
public:

  DescribeTableNode(const String& table_name);

  Vector<RefPtr<QueryTreeNode>> inputTables() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  const String& tableName() const;

protected:
  String table_name_;
};

} // namespace csql
