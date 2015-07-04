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
#include <fnord/autoref.h>
#include <fnord/option.h>
#include <chartsql/qtree/QueryTreeNode.h>
#include <chartsql/qtree/GroupByNode.h>
#include <chartsql/qtree/UnionNode.h>
#include <chartsql/qtree/LimitNode.h>
#include <chartsql/qtree/SequentialScanNode.h>
#include <chartsql/qtree/SelectExpressionNode.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/tablerepository.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {
class Runtime;

class TableExpressionBuilder {
public:

  ScopedPtr<TableExpression> build(
      RefPtr<TableExpressionNode> node,
      Runtime* runtime,
      TableProvider* tables);

protected:

  ScopedPtr<TableExpression> buildGroupBy(
      RefPtr<GroupByNode> node,
      Runtime* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildUnion(
      RefPtr<UnionNode> node,
      Runtime* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildLimit(
      RefPtr<LimitNode> node,
      Runtime* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildSequentialScan(
      RefPtr<SequentialScanNode> node,
      Runtime* runtime,
      TableProvider* tables);

  ScopedPtr<TableExpression> buildSelectExpression(
      RefPtr<SelectExpressionNode> node,
      Runtime* runtime,
      TableProvider* tables);

};

} // namespace csql
