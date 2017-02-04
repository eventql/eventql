/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <paul@eventql.io>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/ColumnReferenceNode.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/qtree/CallExpressionNode.h>
#include <eventql/sql/transaction.h>

#include "eventql/eventql.h"

namespace csql {

class QueryTreeUtil {
public:

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
  static ReturnCode prunePredicateExpression(
      Transaction* txn,
      RefPtr<ValueExpressionNode> expr,
      const Set<String>& column_whitelist,
      RefPtr<ValueExpressionNode>* out);

  /**
   * Removes the provided scan constraint from the provided predicate
   * expression
   *
   * This method does _not_ preserve the full correctness of the provided
   * expression: The pruned predicate expression may match on more inputs than
   * the original expression did. However, it is guaranteed that all inputs
   * that match the original expression will still match the pruned expression.
   */
  static ReturnCode removeConstraintFromPredicate(
      Transaction* txn,
      RefPtr<ValueExpressionNode> expr,
      const ScanConstraint& constraint,
      RefPtr<ValueExpressionNode>* out);

  static const CallExpressionNode* findAggregateExpression(
      const ValueExpressionNode* expr);

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

  /**
   * Walk the query tree (depth first) and returns a pointer to the first node
   * that is of type T. If no such node can be found in the query tree, returns
   * nullptr.
   *
   * T* must be convertible to QueryTreeNode*
   */
  template <class T>
  static T* findNode(QueryTreeNode* tree);

};

} // namespace csql

#include "eventql/sql/qtree/QueryTreeUtil_impl.h"
