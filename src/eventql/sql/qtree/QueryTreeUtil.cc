/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/util/logging.h>

using namespace stx;

namespace csql {

void QueryTreeUtil::resolveColumns(
    RefPtr<ValueExpressionNode> expr,
    Function<size_t (const String&)> resolver) {
  auto colref = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (colref && !colref->fieldName().empty()) {
    auto idx = resolver(colref->fieldName());
    if (idx == size_t(-1)) {
      RAISEF(kRuntimeError, "column(s) not found: '$0'", colref->fieldName());
    }

    colref->setColumnIndex(idx);
  }

  for (auto& arg : expr->arguments()) {
    resolveColumns(arg, resolver);
  }
}

void QueryTreeUtil::findColumns(
    RefPtr<ValueExpressionNode> expr,
    Function<void (const RefPtr<ColumnReferenceNode>&)> fn) {
  auto colref = dynamic_cast<ColumnReferenceNode*>(expr.get());
  if (colref) {
    fn(colref);
  }

  for (auto& arg : expr->arguments()) {
    findColumns(arg, fn);
  }
}

RefPtr<ValueExpressionNode> QueryTreeUtil::foldConstants(
    Transaction* txn,
    RefPtr<ValueExpressionNode> expr) {
  if (isConstantExpression(txn, expr)) {
    auto runtime = txn->getRuntime();
    auto const_val = runtime->evaluateConstExpression(txn, expr);

    return new LiteralExpressionNode(const_val);
  } else {
    return expr;
  }
}

bool QueryTreeUtil::isConstantExpression(
    Transaction* txn,
    RefPtr<ValueExpressionNode> expr) {
  if (dynamic_cast<ColumnReferenceNode*>(expr.get())) {
    return false;
  }

  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr) {
    auto symbol = txn->getSymbolTable()->lookup(call_expr->symbol());
    if (symbol.isAggregate()) {
      return false;
    }
  }

  for (const auto& arg : expr->arguments()) {
    if (!isConstantExpression(txn, arg)) {
      return false;
    }
  }

  return true;
}

RefPtr<ValueExpressionNode> QueryTreeUtil::prunePredicateExpression(
    RefPtr<ValueExpressionNode> expr,
    const Set<String>& column_whitelist) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr && call_expr->symbol() == "logical_and") {
    return new CallExpressionNode(
        "logical_and",
        Vector<RefPtr<ValueExpressionNode>> {
          prunePredicateExpression(call_expr->arguments()[0], column_whitelist),
          prunePredicateExpression(call_expr->arguments()[1], column_whitelist),
        });
  }

  bool is_invalid = false;
  findColumns(expr, [&] (const RefPtr<ColumnReferenceNode>& col) {
    const auto& col_name = col->columnName();
    if (!col_name.empty() && column_whitelist.count(col_name) == 0) {
      is_invalid = true;
    }
  });

  if (is_invalid) {
    return new LiteralExpressionNode(SValue(SValue::BoolType(true)));
  } else {
    return expr;
  }
}

RefPtr<ValueExpressionNode> QueryTreeUtil::removeConstraintFromPredicate(
    RefPtr<ValueExpressionNode> expr,
    const ScanConstraint& constraint) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr && call_expr->symbol() == "logical_and") {
    return new CallExpressionNode(
        "logical_and",
        Vector<RefPtr<ValueExpressionNode>> {
          removeConstraintFromPredicate(call_expr->arguments()[0], constraint),
          removeConstraintFromPredicate(call_expr->arguments()[1], constraint),
        });
  }

  auto e_constraint = findConstraint(expr);
  if (!e_constraint.isEmpty() && e_constraint.get() == constraint) {
    return new LiteralExpressionNode(SValue(SValue::BoolType(true)));
  } else {
    return expr;
  }
}

void QueryTreeUtil::findConstraints(
    RefPtr<ValueExpressionNode> expr,
    Vector<ScanConstraint>* constraints) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());

  // logical ands allow chaining multiple constraints
  if (call_expr && call_expr->symbol() == "logical_and") {
    for (const auto& arg : call_expr->arguments()) {
      findConstraints(arg, constraints);
    }

    return;
  }

  auto constraint = QueryTreeUtil::findConstraint(expr);
  if (!constraint.isEmpty()) {
    constraints->emplace_back(constraint.get());
  }
}

Option<ScanConstraint> QueryTreeUtil::findConstraint(
    RefPtr<ValueExpressionNode> expr) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr == nullptr) {
    return None<ScanConstraint>();
  }

  RefPtr<LiteralExpressionNode> literal;
  RefPtr<ColumnReferenceNode> column;
  bool reverse_expr = false;
  auto args = expr->arguments();
  if (args.size() == 2) {
    for (size_t i = 0; i < args.size(); ++i) {
      auto literal_expr = dynamic_cast<LiteralExpressionNode*>(args[i].get());
      if (literal_expr) {
        literal = mkRef(literal_expr);
      }
      auto colref_expr = dynamic_cast<ColumnReferenceNode*>(args[i].get());
      if (colref_expr) {
        column = mkRef(colref_expr);
        reverse_expr = i > 0;
      }
    }
  }

  if (literal.get() == nullptr || column.get() == nullptr) {
    return None<ScanConstraint>();
  }

  // EQUAL_TO
  if (call_expr->symbol() == "eq") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = ScanConstraintType::EQUAL_TO;
    constraint.value = literal->value();
    return Some(constraint);
  }

  // NOT_EQUAL_TO
  if (call_expr->symbol() == "neq") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = ScanConstraintType::NOT_EQUAL_TO;
    constraint.value = literal->value();
    return Some(constraint);
  }

  // LESS_THAN
  if (call_expr->symbol() == "lt") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = reverse_expr ?
        ScanConstraintType::GREATER_THAN :
        ScanConstraintType::LESS_THAN;
    constraint.value = literal->value();
    return Some(constraint);
  }

  // LESS_THAN_OR_EQUALS
  if (call_expr->symbol() == "lte") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = reverse_expr ?
        ScanConstraintType::GREATER_THAN_OR_EQUAL_TO :
        ScanConstraintType::LESS_THAN_OR_EQUAL_TO;
    constraint.value = literal->value();
    return Some(constraint);
  }

  // GREATER_THAN
  if (call_expr->symbol() == "gt") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = reverse_expr ?
        ScanConstraintType::LESS_THAN :
        ScanConstraintType::GREATER_THAN;
    constraint.value = literal->value();
    return Some(constraint);
  }

  // GREATER_THAN_OR_EQUAL_TO
  if (call_expr->symbol() == "gte") {
    ScanConstraint constraint;
    constraint.column_name = column->fieldName();
    constraint.type = reverse_expr ?
        ScanConstraintType::LESS_THAN_OR_EQUAL_TO :
        ScanConstraintType::GREATER_THAN_OR_EQUAL_TO;
    constraint.value = literal->value();
    return Some(constraint);
  }

  return None<ScanConstraint>();
}

} // namespace csql
