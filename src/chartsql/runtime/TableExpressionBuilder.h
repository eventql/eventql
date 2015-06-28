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
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/TableRepository.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {
class DefaultRuntime;

class TableExpressionBuilder {
public:

  ScopedPtr<TableExpression> build(
      RefPtr<TableExpressionNode> node,
      DefaultRuntime* runtime,
      TableRepository* tables);

protected:

  ScopedPtr<TableExpression> buildGroupBy(
      RefPtr<GroupByNode> node,
      DefaultRuntime* runtime,
      TableRepository* tables);

};

} // namespace csql
