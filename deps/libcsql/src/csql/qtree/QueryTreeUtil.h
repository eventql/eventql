/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <csql/qtree/ValueExpressionNode.h>
#include <csql/qtree/ColumnReferenceNode.h>
#include <csql/qtree/SequentialScanNode.h>
#include <csql/Transaction.h>

using namespace stx;

namespace csql {

class QueryTreeUtil {
public:

  /**
   * Walks the provided value expression and calls the provided resolver
   * function for each unresolved column name. The resolver must return a column
   * index for each column name
   *
   * This method will modify the provided expression in place
   */
  static void resolveColumns(
      RefPtr<ValueExpressionNode> expr,
      Function<size_t (const String&)> resolver);

  /**
   * Walks the provided value expression and calls the provided function for
   * each column reference
   *
   * The function may modify the provided expression in place
   */
  static void findColumns(
      RefPtr<ValueExpressionNode> expr,
      Function<void (const RefPtr<ColumnReferenceNode>&)> fn);

  /**
   * Walks the provided value expression and folds all constant subexpressions
   * into a literal (by evaluating them)
   */
  static RefPtr<ValueExpressionNode> foldConstants(
      Transaction* txn,
      RefPtr<ValueExpressionNode> expr);

  static bool isConstantExpression(
      Transaction* txn,
      RefPtr<ValueExpressionNode> expr);

  /**
   * Prunes the provided predicate expression such that all references
   * to columns no in the provided whitelist are removed.
   *
   * This method does _not_ preserve the full correctness of the provided
   * expression: The pruned predicate expression may match on more inputs than
   * the original expression did. However, it is guaranteed that all inputs
   * that match the original expression will still match the pruned expression.
   */
  static RefPtr<ValueExpressionNode> prunePredicateExpression(
      RefPtr<ValueExpressionNode> expr,
      const Set<String>& column_whitelist);

  /**
   * Removes the provided scan constraint from the provided predicate
   * expression
   *
   * This method does _not_ preserve the full correctness of the provided
   * expression: The pruned predicate expression may match on more inputs than
   * the original expression did. However, it is guaranteed that all inputs
   * that match the original expression will still match the pruned expression.
   */
  static RefPtr<ValueExpressionNode> removeConstraintFromPredicate(
      RefPtr<ValueExpressionNode> expr,
      const ScanConstraint& constraint);

  /**
   * Extracts all constraints from the provided predicate expression
   */
  static void findConstraints(
      RefPtr<ValueExpressionNode> expr,
      Vector<ScanConstraint>* constraints);

  /**
   * Extract a single ScanConstraint from an expression of the format
   * "column <OP> constvalue" or "constvalue <OP> columns"
   */
  static Option<ScanConstraint> findConstraint(
      RefPtr<ValueExpressionNode> expr);
};


} // namespace csql
