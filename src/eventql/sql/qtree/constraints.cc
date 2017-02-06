
/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/qtree/constraints.h>

namespace csql {

void findJoinConjunctions(
    const JoinNode* join,
    const ValueExpressionNode* expr,
    std::vector<JoinConjunction>* conjunctions) {
  auto call_expr = dynamic_cast<const CallExpressionNode*>(expr);
  if (!call_expr) {
    return;
  }

  // logical ands allow chaining multiple conjunctions
  if (call_expr->getFunctionName() == "logical_and") {
    for (const auto& arg : call_expr->arguments()) {
      findJoinConjunctions(join, arg.get(), conjunctions);
    }

    return;
  }

  // check if this function is a candidate
  JoinConjunctionType conjunction_type;
  if (call_expr->getFunctionName() == "eq") {
    conjunction_type = JoinConjunctionType::EQUAL_TO;
  } else if (call_expr->getFunctionName() == "neq") {
    conjunction_type = JoinConjunctionType::NOT_EQUAL_TO;
  } else if (call_expr->getFunctionName() == "lt") {
    conjunction_type = JoinConjunctionType::LESS_THAN;
  } else if (call_expr->getFunctionName() == "lte") {
    conjunction_type = JoinConjunctionType::LESS_THAN_OR_EQUAL_TO;
  } else if (call_expr->getFunctionName() == "gt") {
    conjunction_type = JoinConjunctionType::GREATER_THAN;
  } else if (call_expr->getFunctionName() == "gte") {
    conjunction_type = JoinConjunctionType::GREATER_THAN_OR_EQUAL_TO;
  } else {
    return;
  }

  assert(call_expr->arguments().size() == 2);

  std::set<size_t> left_tables;
  auto left_expr = call_expr->arguments()[0];
  findJoinInputDependencies(join, left_expr.get(), &left_tables);

  std::set<size_t> right_tables;
  auto right_expr = call_expr->arguments()[1];
  findJoinInputDependencies(join, right_expr.get(), &right_tables);

  if (left_tables.size() != 1 ||
      right_tables.size() != 1 ||
      left_tables == right_tables) {
    return;
  }

  JoinConjunction conjunction;
  conjunction.type = conjunction_type;
  if (left_tables.count(0) > 0) {
    conjunction.base_table_expr = left_expr;
    conjunction.joined_table_expr = right_expr;
  } else {
    conjunction.base_table_expr = right_expr;
    conjunction.joined_table_expr = left_expr;
  }

  conjunctions->emplace_back(std::move(conjunction));
}

void findJoinInputDependencies(
    const JoinNode* join,
    const ValueExpressionNode* expr,
    std::set<size_t>* tables) {
  auto colref = dynamic_cast<const ColumnReferenceNode*>(expr);
  if (colref) {
    auto colidx = colref->columnIndex();
    tables->insert(join->inputColumnMap()[colidx].table_idx);
  }

  for (const auto& arg : expr->arguments()) {
    findJoinInputDependencies(join, arg.get(), tables);
  }
}

} // namespace csql

