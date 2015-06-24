/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/CSTableScan.h>
#include <chartsql/qtree/FieldReferenceNode.h>
#include <fnord/inspect.h>

using namespace fnord;

namespace csql {

CSTableScan::CSTableScan(
    RefPtr<SelectProjectAggregateNode> stmt,
    cstable::CSTableReader&& cstable) :
    cstable_(std::move(cstable)){

  Set<String> column_names;
  for (const auto& expr : stmt->selectList()) {
    findColumns(expr, &column_names);
  }

  for (const auto& col : column_names) {
    columns_.emplace(col, cstable_.getColumnReader(col));
  }

  //fnord::iputs("all cols: $0", column_names);

  for (const auto& expr : stmt->selectList()) {
    ExpressionRef expr_ref;
    expr_ref.rep_level = findMaxRepetitionLevel(expr);
    select_list_.emplace_back(expr_ref);
  }
}

void CSTableScan::execute(Function<bool (int argc, const SValue* argv)> fn) {
  RAISE(kNotYetImplementedError);
}

void CSTableScan::findColumns(
    RefPtr<ScalarExpressionNode> expr,
    Set<String>* column_names) const {
  auto fieldref = dynamic_cast<FieldReferenceNode*>(expr.get());
  if (fieldref != nullptr) {
    column_names->emplace(fieldref->fieldName());
  }

  for (const auto& e : expr->arguments()) {
    findColumns(e, column_names);
  }
}

uint64_t CSTableScan::findMaxRepetitionLevel(
    RefPtr<ScalarExpressionNode> expr) const {
  uint64_t max_level = 0;

  auto fieldref = dynamic_cast<FieldReferenceNode*>(expr.get());
  if (fieldref != nullptr) {
    auto col = columns_.find(fieldref->fieldName());
    if (col == columns_.end()) {
      RAISE(kIllegalStateError);
    }

    auto col_level = col->second->maxRepetitionLevel();
    if (col_level > max_level) {
      max_level = col_level;
    }
  }

  for (const auto& e : expr->arguments()) {
    auto e_level = findMaxRepetitionLevel(e);
    if (e_level > max_level) {
      max_level = e_level;
    }
  }

  return max_level;
}

} // namespace csql
