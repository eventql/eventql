/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/CSTableScanBuildRule.h>
#include <chartsql/CSTableScan.h>
#include <chartsql/qtree/SequentialScanNode.h>

using namespace fnord;

namespace csql {

CSTableScanBuildRule::CSTableScanBuildRule(
    const String& table_name,
    const String& cstable_file) :
    table_name_(table_name),
    cstable_file_(cstable_file) {}

Option<ScopedPtr<TableExpression>> CSTableScanBuildRule::build(
      RefPtr<TableExpressionNode> node,
      DefaultRuntime* runtime) const {
  auto seqscan = dynamic_cast<SequentialScanNode*>(node.get());
  if (!seqscan || seqscan->tableName() != table_name_) {
    return None<ScopedPtr<TableExpression>>();
  }

  return std::move(
      Option<ScopedPtr<TableExpression>>(
          mkScoped(
              new CSTableScan(
                  seqscan,
                  cstable::CSTableReader(cstable_file_)))));
}

} // namespace csql
