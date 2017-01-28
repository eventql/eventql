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
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/util/logging.h>

#include "eventql/eventql.h"

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
    if (symbol.isAggregate() || symbol.hasSideEffects()) {
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

ReturnCode QueryTreeUtil::prunePredicateExpression(
    RefPtr<ValueExpressionNode> expr,
    const Set<String>& column_whitelist,
    RefPtr<ValueExpressionNode>* out) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr && call_expr->symbol() == "logical_and") {
    Vector<RefPtr<ValueExpressionNode>> call_args(2);

    {
      auto rc = prunePredicateExpression(
          call_expr->arguments()[0],
          column_whitelist,
          &call_args[0]);

      if (rc.isSuccess()) {
        return rc;
      }
    }

    {
      auto rc = prunePredicateExpression(
          call_expr->arguments()[1],
          column_whitelist,
          &call_args[1]);

      if (rc.isSuccess()) {
        return rc;
      }
    }

    return CallExpressionNode::newNode("logical_and", call_args, out);
  }

  bool is_invalid = false;
  findColumns(expr, [&] (const RefPtr<ColumnReferenceNode>& col) {
    const auto& col_name = col->columnName();
    if (!col_name.empty() && column_whitelist.count(col_name) == 0) {
      is_invalid = true;
    }
  });

  if (is_invalid) {
    *out = RefPtr<ValueExpressionNode>(
        new LiteralExpressionNode(SValue(SValue::BoolType(true))));
  } else {
    *out = expr;
  }

  return ReturnCode::success();
}

ReturnCode QueryTreeUtil::removeConstraintFromPredicate(
    RefPtr<ValueExpressionNode> expr,
    const ScanConstraint& constraint,
    RefPtr<ValueExpressionNode>* out) {
  auto call_expr = dynamic_cast<CallExpressionNode*>(expr.get());
  if (call_expr && call_expr->symbol() == "logical_and") {
    RefPtr<ValueExpressionNode> arg_left;
    {
      auto rc = removeConstraintFromPredicate(
          call_expr->arguments()[0],
          constraint,
          &arg_left);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    RefPtr<ValueExpressionNode> arg_right;
    {
      auto rc = removeConstraintFromPredicate(
          call_expr->arguments()[1],
          constraint,
          &arg_right);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    auto arg_left_is_true =
        dynamic_cast<LiteralExpressionNode*>(arg_left.get()) &&
        dynamic_cast<LiteralExpressionNode*>(arg_left.get())->value().isBool() &&
        dynamic_cast<LiteralExpressionNode*>(arg_left.get())->value().getBool();

    auto arg_right_is_true =
        dynamic_cast<LiteralExpressionNode*>(arg_right.get()) &&
        dynamic_cast<LiteralExpressionNode*>(arg_right.get())->value().isBool() &&
        dynamic_cast<LiteralExpressionNode*>(arg_right.get())->value().getBool();

    if (arg_left_is_true && arg_right_is_true) {
      *out = RefPtr<ValueExpressionNode>(
          new LiteralExpressionNode(SValue(SValue::BoolType(true))));
      return ReturnCode::success();
    } else if (arg_left_is_true) {
      *out = arg_right;
      return ReturnCode::success();
    } else if (arg_right_is_true) {
      *out = arg_left;
      return ReturnCode::success();
    } else {
      return CallExpressionNode::newNode(
          "logical_and",
          Vector<RefPtr<ValueExpressionNode>> { arg_left, arg_right },
          out);
    }
  }

  auto e_constraint = findConstraint(expr);
  if (!e_constraint.isEmpty() && e_constraint.get() == constraint) {
    *out = RefPtr<ValueExpressionNode>(
        new LiteralExpressionNode(SValue(SValue::BoolType(true))));
  } else {
    *out = expr;
  }

  return ReturnCode::success();
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
