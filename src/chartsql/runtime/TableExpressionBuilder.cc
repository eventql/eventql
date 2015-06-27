/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/groupby.h>

using namespace fnord;

namespace csql {

ScopedPtr<TableExpression> TableExpressionBuilder::build(
    RefPtr<TableExpressionNode> node,
    DefaultRuntime* runtime) {

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return mkScoped(new GroupBy(node.asInstanceOf<GroupByNode>(), runtime));
  }

  for (const auto& rule : rules_) {
    auto opt = rule->build(node, runtime);
    if (!opt.isEmpty()) {
      return std::move(opt.get());
    }
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a table expression for this QTree node");
}

void TableExpressionBuilder::addBuildRule(RefPtr<BuildRule> rule) {
  rules_.emplace_back(rule);
}

} // namespace csql
