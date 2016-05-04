/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <csql/parser/astnode.h>
#include <csql/parser/astutil.h>
#include <csql/parser/parser.h>
#include <csql/runtime/queryplanbuilder.h>
#include <csql/qtree/GroupByNode.h>
#include <csql/qtree/IfExpressionNode.h>
#include <csql/qtree/SelectExpressionNode.h>
#include <csql/qtree/LimitNode.h>
#include <csql/qtree/OrderByNode.h>
#include <csql/qtree/DrawStatementNode.h>
#include <csql/qtree/ChartStatementNode.h>
#include <csql/qtree/ShowTablesNode.h>
#include <csql/qtree/DescribeTableNode.h>
#include <csql/qtree/RegexExpressionNode.h>
#include <csql/qtree/LikeExpressionNode.h>
#include <csql/qtree/SubqueryNode.h>
#include <csql/qtree/QueryTreeUtil.h>
#include <csql/qtree/ValueExpressionNode.h>
#include <csql/qtree/JoinNode.h>

namespace csql {

QueryPlanBuilder::QueryPlanBuilder(
    QueryPlanBuilderOptions opts,
    SymbolTable* symbol_table) :
    symbol_table_(symbol_table) {}

RefPtr<QueryTreeNode> QueryPlanBuilder::build(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  QueryTreeNode* node = nullptr;

  /* assign explicit column names to all output columns */
  if (hasImplicitlyNamedColumns(ast)) {
    assignExplicitColumnNames(txn, ast, tables);
  }

  /* internal nodes: multi table query (joins), order, aggregation, limit */
  if ((node = buildLimitClause(txn, ast, tables)) != nullptr) {
    return node;
  }

  if (hasOrderByClause(ast)) {
    return buildOrderByClause(txn, ast, tables);
  }

  if (hasGroupByClause(ast) || hasAggregationInSelectList(ast)) {
    return buildGroupBy(txn, ast, tables);
  }

  /* leaf nodes: table scan, join, subquery tableless select */
  if ((node = buildJoin(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSubquery(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSequentialScan(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSelectExpression(txn, ast)) != nullptr) {
    return node;
  }

  /* other statments */
  if ((node = buildShowTables(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildDescribeTable(txn, ast)) != nullptr) {
    return node;
  }

  ast->debugPrint(2);
  RAISE(kRuntimeError, "can't figure out a query plan for this, sorry :(");
}

Vector<RefPtr<QueryTreeNode>> QueryPlanBuilder::build(
    Transaction* txn,
    const Vector<ASTNode*>& statements,
    RefPtr<TableProvider> tables) {
  Vector<RefPtr<QueryTreeNode>> nodes;

  for (int i = 0; i < statements.size(); ++i) {
    switch (statements[i]->getType()) {

      case ASTNode::T_SELECT:
      case ASTNode::T_SELECT_DEEP:
      case ASTNode::T_SHOW_TABLES:
      case ASTNode::T_DESCRIBE_TABLE:
        nodes.emplace_back(build(txn, statements[i], tables));
        break;

      case ASTNode::T_DRAW: {
        Vector<RefPtr<QueryTreeNode>> draw_nodes;

        while (i < statements.size() &&
            statements[i]->getType() == ASTNode::T_DRAW) {
          ScopedPtr<ASTNode> draw_ast(statements[i]->deepCopy());
          Vector<RefPtr<QueryTreeNode>> subselects;

          for (++i; i < statements.size(); ) {
            switch (statements[i]->getType()) {
              case ASTNode::T_SELECT:
              case ASTNode::T_SELECT_DEEP:
                subselects.emplace_back(build(txn, statements[i++], tables));
                continue;
              case ASTNode::T_DRAW:
                break;
              default:
                RAISE(
                    kRuntimeError,
                    "DRAW statments may only be followed by SELECT or END DRAW " \
                    "statements");
            }

            break;
          }

          draw_nodes.emplace_back(
              new DrawStatementNode(std::move(draw_ast), subselects));
        }

        nodes.emplace_back(new ChartStatementNode(draw_nodes));
        break;
      }

      default:
        statements[i]->debugPrint();
        RAISE(kRuntimeError, "invalid statement");
    }
  }

  return nodes;
}

//QueryPlanBuilder::QueryPlanBuilder(
//    ValueExpressionBuilder* compiler,
//    const std::vector<std::unique_ptr<Backend>>& backends) :
//    QueryPlanBuilderInterface(compiler, backends) {}

//void QueryPlanBuilder::buildQueryPlan(
//    const std::vector<std::unique_ptr<ASTNode>>& statements,
//    QueryPlan* query_plan) {
//
//  for (const auto& stmt : statements) {
//    switch (stmt->getType()) {
//      case csql::ASTNode::T_SELECT: {
//        auto query_plan_node = buildQueryPlan(
//            stmt.get(),
//            query_plan->tableRepository());
//
//        if (query_plan_node == nullptr) {
//          RAISE(
//              kRuntimeError,
//              "can't figure out a query plan for this, sorry :(");
//        }
//
//        query_plan->addQuery(std::unique_ptr<QueryPlanNode>(query_plan_node));
//        break;
//      }
//
//      case csql::ASTNode::T_IMPORT:
//        query_plan->tableRepository()->import(
//            ImportStatement(stmt.get(), compiler_),
//            backends_);
//        break;
//
//      default:
//        RAISE(kRuntimeError, "invalid statement");
//    }
//  }
//}
//
//QueryPlanNode* QueryPlanBuilder::buildQueryPlan(
//    ASTNode* ast,
//    TableRepository* repo) {
//  QueryPlanNode* exec = nullptr;
///  // if verbose -> dump ast
//  return nullptr;
//}
//

bool QueryPlanBuilder::hasImplicitlyNamedColumns(ASTNode* ast) const {
  if (ast->getType() != ASTNode::T_SELECT &&
      ast->getType() != ASTNode::T_SELECT_DEEP) {
    return false;
  }

  if (ast->getChildren().size() < 1 ||
      ast->getChildren()[0]->getType() != ASTNode::T_SELECT_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (ast->getChildren().size() == 1) {
    return false;
  }

  for (const auto& col : ast->getChildren()[0]->getChildren()) {
    if (col->getType() == ASTNode::T_DERIVED_COLUMN &&
        col->getChildren().size() == 1) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasGroupByClause(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
    return false;
  }

  for (const auto& child : ast->getChildren()) {
    if (child->getType() == ASTNode::T_GROUP_BY) {
      return true;
    }
  }

  return false;
}
//
//bool QueryPlanBuilder::hasGroupOverTimewindowClause(ASTNode* ast) const {
//  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
//    return false;
//  }
//
//  for (const auto& child : ast->getChildren()) {
//    if (child->getType() == ASTNode::T_GROUP_OVER_TIMEWINDOW) {
//      return true;
//    }
//  }
//
//  return false;
//}
//
//
bool QueryPlanBuilder::hasJoin(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
    return false;
  }

  auto from_list = ast->getChildren()[1];
  if (from_list->getType() != ASTNode::T_FROM ||
      from_list->getChildren().size() < 1) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (from_list->getChildren().size() > 1) {
    return true;
  }

  //for (const auto& child : ast->getChildren()) {
    //if (child->getType() == ASTNode::T_JOIN) {
    //  return true;
    //}
  //}

  return false;
}

bool QueryPlanBuilder::hasOrderByClause(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 2) {
    return false;
  }

  for (const auto& child : ast->getChildren()) {
    if (child->getType() == ASTNode::T_ORDER_BY) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasAggregationInSelectList(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 2) {
    return false;
  }

  auto select_list = ast->getChildren()[0];
  if (!(select_list->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return hasAggregationExpression(select_list);
}

bool QueryPlanBuilder::hasAggregationExpression(ASTNode* ast) const {
  if (ast->getType() == ASTNode::T_METHOD_CALL) {
    if (!(ast->getToken() != nullptr)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (symbol_table_->isAggregateFunction(ast->getToken()->getString())) {
      return true;
    }
  }

  for (const auto& child : ast->getChildren()) {
    if (hasAggregationExpression(child)) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasAggregationWithinRecord(ASTNode* ast) const {
  if (ast->getType() == ASTNode::T_METHOD_CALL_WITHIN_RECORD) {
    return true;
  }

  for (const auto& child : ast->getChildren()) {
    if (hasAggregationWithinRecord(child)) {
      return true;
    }
  }

  return false;
}

void QueryPlanBuilder::assignExplicitColumnNames(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (ast->getChildren().size() < 1) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0];
  if (select_list->getType() != ASTNode::T_SELECT_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  for (const auto& col : select_list->getChildren()) {
    if (col->getType() == ASTNode::T_DERIVED_COLUMN &&
        col->getChildren().size() == 1) {
      auto alias = col->appendChild(ASTNode::T_COLUMN_ALIAS);
      alias->setToken(
          new Token(
              Token::T_IDENTIFIER,
              ASTUtil::columnNameForExpression(col->getChildren()[0])));
    }
  }
}

QueryTreeNode* QueryPlanBuilder::buildGroupBy(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  /* copy own select list */
  if (!(ast->getChildren()[0]->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0]->deepCopy();

  /* generate select list for child */
  auto child_sl = new ASTNode(ASTNode::T_SELECT_LIST);
  buildGroupBySelectList(select_list, child_sl);

  /* search for a group by clause */
  Vector<RefPtr<ValueExpressionNode>> group_expressions;
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_GROUP_BY) {
      continue;
    }

    for (const auto& group_expr : child->getChildren()) {
      auto e = group_expr->deepCopy();
      if (hasAggregationExpression(e)) {
        RAISE(kRuntimeError, "GROUP clause can only contain pure functions");
      }

      group_expressions.emplace_back(buildValueExpression(txn, e));
    }
  }

  /* copy ast for child and swap out select lists*/
  auto child_ast = ast->deepCopy();
  child_ast->removeChildrenByType(ASTNode::T_GROUP_BY);
  child_ast->removeChildByIndex(0);
  child_ast->appendChild(child_sl, 0);

  auto subtree = build(txn, child_ast, tables);
  auto subtree_tbl = subtree.asInstanceOf<TableExpressionNode>();

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : subtree_tbl->allColumns()) {
        auto sl = new SelectListNode(new ColumnReferenceNode(col.qualified_name));
        sl->setAlias(col.short_name);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      auto sl = buildSelectList(txn, select_expr);
      select_list_expressions.emplace_back(sl);
    }
  }

  for (auto& sl : select_list_expressions) {
    QueryTreeUtil::resolveColumns(
        sl->expression(),
        std::bind(
            &TableExpressionNode::getColumnIndex,
            subtree_tbl.get(),
            std::placeholders::_1,
            false));
  }

  for (auto& e : group_expressions) {
    QueryTreeUtil::resolveColumns(
        e,
        std::bind(
            &TableExpressionNode::getColumnIndex,
            subtree_tbl.get(),
            std::placeholders::_1,
            true));
  }

  return new GroupByNode(
      select_list_expressions,
      group_expressions,
      subtree);
}

bool QueryPlanBuilder::buildGroupBySelectList(
    ASTNode* node,
    ASTNode* target_select_list) {
  /* search recursively */
  switch (node->getType()) {

    /* push down WITHIN RECORD aggregations into child select list */
    case ASTNode::T_METHOD_CALL_WITHIN_RECORD: {
      auto derived = new ASTNode(ASTNode::T_DERIVED_COLUMN);
      derived->appendChild(node->deepCopy());
      target_select_list->appendChild(derived);
      auto col_index = target_select_list->getChildren().size() - 1;
      node->setType(ASTNode::T_RESOLVED_COLUMN);
      node->setID(col_index);
      node->clearChildren();
      node->clearToken();
      return true;
    }

    /* push down referenced columns into the child select list */
    case ASTNode::T_COLUMN_NAME: {
      auto derived = new ASTNode(ASTNode::T_DERIVED_COLUMN); // LEAK
      derived->appendChild(node->deepCopy());

      /* check if this column already exists in the select list */
      auto col_index = -1;
      const auto& candidates = target_select_list->getChildren();
      for (int i = 0; i < candidates.size(); ++i) {
        if (derived->compare(candidates[i])) {
          col_index = i;
          break;
        }
      }

      /* otherwise add this column to the select list */
      if (col_index < 0) {
        target_select_list->appendChild(derived);
        col_index = target_select_list->getChildren().size() - 1;
      }

      node->setType(ASTNode::T_RESOLVED_COLUMN);
      node->setID(col_index);
      node->clearChildren();
      node->clearToken();
      return true;
    }

    /* push down aggregate function arguments */
    case ASTNode::T_METHOD_CALL:
      if (node->getToken() == nullptr) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      if (symbol_table_->isAggregateFunction(node->getToken()->getString())) {
        auto derived = new ASTNode(ASTNode::T_DERIVED_COLUMN);
        for (auto& cld : node->getChildren()) {
          derived->appendChild(cld->deepCopy());
          target_select_list->appendChild(derived);
          auto col_index = target_select_list->getChildren().size() - 1;
          cld->setType(ASTNode::T_RESOLVED_COLUMN);
          cld->setID(col_index);
          cld->clearChildren();
          cld->clearToken();
        }

        return true;
      } else {
        /* fallthrough */
      }

    default: {
      for (const auto& child : node->getChildren()) {
        if (!buildGroupBySelectList(child, target_select_list)) {
          return false;
        }
      }

      return true;
    }

  }
}

QueryTreeNode* QueryPlanBuilder::buildLimitClause(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 3) {
    return nullptr;
  }

  for (const auto& child : ast->getChildren()) {
    int limit = 0;
    int offset = 0;

    if (child->getType() != ASTNode::T_LIMIT) {
      continue;
    }

    auto limit_token = child->getToken();
    if (!(limit_token)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (!(*limit_token == Token::T_NUMERIC)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    limit = limit_token->getInteger();

    if (child->getChildren().size() == 1) {
      if (!(child->getChildren()[0]->getType() == ASTNode::T_OFFSET)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto offset_token = child->getChildren()[0]->getToken();
      if (!(offset_token)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      if (!(*offset_token == Token::T_NUMERIC)) {
        RAISE(kRuntimeError, "corrupt AST");
      }
      offset = offset_token->getInteger();
    }

    // clone ast + remove limit clause
    auto new_ast = ast->deepCopy();
    new_ast->removeChildrenByType(ASTNode::T_LIMIT);

    return new LimitNode(
        limit,
        offset,
        build(txn, new_ast, tables));
  }

  return nullptr;
}

QueryTreeNode* QueryPlanBuilder::buildOrderByClause(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  Vector<OrderByNode::SortSpec> sort_specs;

  /* copy ast for child and remove order by clause */
  auto child_ast = ast->deepCopy();
  child_ast->removeChildrenByType(ASTNode::T_ORDER_BY);
  auto subtree = build(txn, child_ast, tables);
  auto subtree_tbl = subtree.asInstanceOf<TableExpressionNode>();

  /* search for the order by clause */
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_ORDER_BY) {
      continue;
    }

    /* build each sort spec and expand select list for missing columns */
    auto sort_specs_asts = child->getChildren();
    for (int i = 0; i < sort_specs_asts.size(); ++i) {
      auto sort = sort_specs_asts[i];

      auto sort_descending = sort->getToken() != nullptr &&
          sort->getToken()->getType() == Token::T_DESC;

      /* create the sort spec */
      OrderByNode::SortSpec sort_spec;
      sort_spec.expr = buildValueExpression(txn, sort->getChildren()[0]);
      QueryTreeUtil::resolveColumns(
          sort_spec.expr, 
          std::bind(
              &TableExpressionNode::getColumnIndex,
              subtree_tbl.get(),
              std::placeholders::_1,
              true));
      sort_spec.descending = sort_descending;
      sort_specs.emplace_back(sort_spec);
    }
  }

  return new OrderByNode(
      sort_specs,
      subtree);
}

QueryTreeNode* QueryPlanBuilder::buildSequentialScan(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* from_list = ast->getChildren()[1];
  if (!(from_list)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (!(*from_list == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (from_list->getChildren().size() < 1) {
    return nullptr;
  }

  /* get table reference */
  auto tbl_name = from_list->getChildren()[0];
  if (!(*tbl_name == ASTNode::T_TABLE_NAME)) {
    return nullptr;
  }

  /* get select list */
  if (!(*ast->getChildren()[0] == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto select_list = ast->getChildren()[0];

  /* get where clause */
  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildSeqscanTableReference(
      txn,
      from_list,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildSubquery(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* from_list = ast->getChildren()[1];
  if (!(from_list)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (!(*from_list == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (from_list->getChildren().size() < 1) {
    return nullptr;
  }

  if (!(*from_list->getChildren()[0] == ASTNode::T_SELECT)) {
    return nullptr;
  }

  /* get select list */
  if (!(*ast->getChildren()[0] == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto select_list = ast->getChildren()[0];

  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildSubqueryTableReference(
      txn,
      from_list,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildSelectExpression(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)
      || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto select_list = ast->getChildren()[0];

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      RAISE(
          kRuntimeError,
          "Illegal use of wildcard * in free SELECT expression");
    }

    if (hasAggregationExpression(select_expr) ||
        hasAggregationWithinRecord(select_expr)) {
      RAISE(
          kRuntimeError,
          "a SELECT without any tables can only contain pure functions");
    }

    select_list_expressions.emplace_back(buildSelectList(txn, select_expr));
  }

  return new SelectExpressionNode(select_list_expressions);
}

QueryTreeNode* QueryPlanBuilder::buildJoin(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* join = ast->getChildren()[1];
  if (!(join)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  switch (join->getType()) {
    case ASTNode::T_INNER_JOIN:
    case ASTNode::T_LEFT_JOIN:
    case ASTNode::T_RIGHT_JOIN:
    case ASTNode::T_NATURAL_INNER_JOIN:
    case ASTNode::T_NATURAL_LEFT_JOIN:
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      break;
    default:
      return nullptr;
  }

  /* select list  */
  auto select_list = ast->getChildren()[0];
  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildJoinTableReference(
      txn,
      join,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  switch (table_ref->getType()) {
    case ASTNode::T_INNER_JOIN:
    case ASTNode::T_LEFT_JOIN:
    case ASTNode::T_RIGHT_JOIN:
    case ASTNode::T_NATURAL_INNER_JOIN:
    case ASTNode::T_NATURAL_LEFT_JOIN:
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      return buildJoinTableReference(
          txn,
          table_ref,
          select_list,
          where_clause,
          tables,
          in_join);

    case ASTNode::T_FROM:
      if (table_ref->getChildren().size() > 0) {
        switch (table_ref->getChildren()[0]->getType()) {
          case ASTNode::T_TABLE_NAME:
            return buildSeqscanTableReference(
                txn,
                table_ref,
                select_list,
                where_clause,
                tables,
                in_join);

          case ASTNode::T_SELECT:
            return buildSubqueryTableReference(
                txn,
                table_ref,
                select_list,
                where_clause,
                tables,
                in_join);

          default:
            break;

        }
      }
      /* fallthrough */

    default:
      table_ref->debugPrint();
      RAISE(kRuntimeError, "invalid table reference");

  }
}

QueryTreeNode* QueryPlanBuilder::buildJoinTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (table_ref->getChildren().size() < 2) {
    return nullptr;
  }

  JoinType join_type;
  bool natural_join = false;
  bool reverse = false;

  switch (table_ref->getType()) {
    case ASTNode::T_NATURAL_INNER_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_INNER_JOIN:
      join_type = JoinType::INNER;
      break;
    case ASTNode::T_NATURAL_LEFT_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_LEFT_JOIN:
      join_type = JoinType::OUTER;
      break;
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_RIGHT_JOIN:
      reverse = true;
      join_type = JoinType::OUTER;
      break;
    default:
      RAISE(kRuntimeError, "invalid JOIN type");
  }

  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (!in_join && where_clause) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];
    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    where_expr = Some(RefPtr<ValueExpressionNode>(buildValueExpression(txn, e)));
  }

  auto child_sl = mkScoped(new ASTNode(ASTNode::T_SELECT_LIST));

  auto base_table = mkRef(buildTableReference(
      txn,
      table_ref->getChildren()[0],
      child_sl.get(),
      where_clause,
      tables,
      true));

  auto joined_table = mkRef(buildTableReference(
      txn,
      table_ref->getChildren()[1],
      child_sl.get(),
      where_clause,
      tables,
      true));

  Vector<QualifiedColumn> all_columns;
  Option<RefPtr<ValueExpressionNode>> join_cond;

  if (natural_join) {
    RefPtr<TableExpressionNode> primary_table;
    RefPtr<TableExpressionNode> secondary_table;
    if (reverse) {
      primary_table = joined_table.asInstanceOf<TableExpressionNode>();
      secondary_table = base_table.asInstanceOf<TableExpressionNode>();
    } else {
      primary_table = base_table.asInstanceOf<TableExpressionNode>();
      secondary_table = joined_table.asInstanceOf<TableExpressionNode>();
    }

    HashMap<String, Vector<String>> common_columns;
    {
      Set<String> tmp_column_set;
      for (const auto& col : secondary_table->allColumns()) {
        tmp_column_set.insert(col.short_name);
      }

      for (const auto& col : primary_table->allColumns()) {
        if (tmp_column_set.count(col.short_name) > 0) {
          all_columns.emplace_back(col);
          common_columns.emplace(col.short_name, Vector<String>{});
          tmp_column_set.erase(col.short_name);
        }
      }
    }

    for (const auto& col :
            base_table.asInstanceOf<TableExpressionNode>()->allColumns()) {
      if (common_columns.count(col.short_name) == 0) {
        all_columns.emplace_back(col);
      } else {
        common_columns[col.short_name].push_back(col.qualified_name);
      }
    }

    for (const auto& col :
            joined_table.asInstanceOf<TableExpressionNode>()->allColumns()) {
      if (common_columns.count(col.short_name) == 0) {
        all_columns.emplace_back(col);
      } else {
        common_columns[col.short_name].push_back(col.qualified_name);
      }
    }

    RefPtr<ValueExpressionNode> pred;
    for (const auto& c : common_columns) {
      for (size_t i1 = 0; i1 < c.second.size(); ++i1) {
        for (size_t i2 = 0; i2 < c.second.size(); ++i2) {
          if (i1 == i2) {
            continue;
          }

          auto cpred = mkRef<ValueExpressionNode>(
              new csql::CallExpressionNode(
                  "eq",
                  Vector<RefPtr<csql::ValueExpressionNode>>{
                    new csql::ColumnReferenceNode(c.second[i1]),
                    new csql::ColumnReferenceNode(c.second[i2])
                  }));

          if (pred.get() == nullptr) {
            pred = cpred;
          } else {
            pred = mkRef<ValueExpressionNode>(
                new csql::CallExpressionNode(
                    "logical_and",
                    Vector<RefPtr<csql::ValueExpressionNode>>{ pred, cpred }));
          }
        }
      }
    }

    if (pred.get() != nullptr) {
      join_cond = pred;
    }
  } else {
    for (const auto& col :
            base_table.asInstanceOf<TableExpressionNode>()->allColumns()) {
      all_columns.emplace_back(col);
    }

    for (const auto& col :
            joined_table.asInstanceOf<TableExpressionNode>()->allColumns()) {
      all_columns.emplace_back(col);
    }

    if (table_ref->getChildren().size() > 2) {
      auto cond_ast = table_ref->getChildren()[2];

      switch (cond_ast->getType()) {
        case ASTNode::T_JOIN_CONDITION: {
          if (cond_ast->getChildren().size() != 1) {
            RAISE(kRuntimeError, "corrupt AST");
          }

          auto e = cond_ast->getChildren()[0];
          if (e == nullptr) {
            RAISE(kRuntimeError, "corrupt AST");
          }

          if (hasAggregationExpression(e)) {
            RAISE(
                kRuntimeError,
                "JOIN conditions can only contain pure functions\n");
          }

          join_cond = buildValueExpression(txn, e);
          break;
        }

        case ASTNode::T_JOIN_COLUMNLIST: {
          RAISE(kNotYetImplementedError);
        }

        default:
          RAISE(kRuntimeError, "corrupt AST");
      }
    }
  }

  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (hasAggregationWithinRecord(select_expr)) {
      RAISE(
          kRuntimeError,
          "WITHIN RECORD can't be used together with JOIN in the same SELECT"
          " statement. consider moving the WITHIN RECORD expression into a"
          " subquery");
    }

    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : all_columns) {
        auto sl = new SelectListNode(new ColumnReferenceNode(col.qualified_name));
        sl->setAlias(col.short_name);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      select_list_expressions.emplace_back(buildSelectList(txn, select_expr));
    }
  }

  if (join_cond.isEmpty() && join_type == JoinType::INNER) {
    join_type = JoinType::CARTESIAN;
  }

  auto join_node = mkScoped(new JoinNode(
      join_type,
      reverse ? joined_table : base_table,
      reverse ? base_table : joined_table,
      select_list_expressions,
      where_expr,
      join_cond));

  for (auto& sl : join_node->selectList()) {
    QueryTreeUtil::resolveColumns(
        sl->expression(),
        std::bind(
            &JoinNode::getInputColumnIndex,
            join_node.get(),
            std::placeholders::_1,
            false));
  }

  auto jce = join_node->joinCondition();
  if (!jce.isEmpty()) {
    QueryTreeUtil::resolveColumns(
        jce.get(),
        std::bind(
            &JoinNode::getInputColumnIndex,
            join_node.get(),
            std::placeholders::_1,
            true));
  }

  auto we = join_node->whereExpression();
  if (!we.isEmpty()) {
    QueryTreeUtil::resolveColumns(
        we.get(),
        std::bind(
            &JoinNode::getInputColumnIndex,
            join_node.get(),
            std::placeholders::_1,
            true));
  }

  return join_node.release();
}

QueryTreeNode* QueryPlanBuilder::buildSubqueryTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (!(*table_ref == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (table_ref->getChildren().size() < 1) {
    return nullptr;
  }

  /* get subquery */
  auto subquery_ast = table_ref->getChildren()[0];
  if (!(*subquery_ast == ASTNode::T_SELECT)) {
    return nullptr;
  }

  String subquery_alias;
  /* .. AS alias */
  if (table_ref->getChildren().size() > 1 &&
      table_ref->getChildren()[1]->getType() == ASTNode::T_TABLE_ALIAS) {
    subquery_alias = table_ref->getChildren()[1]->getToken()->getString();
  }

  auto subquery = build(txn, subquery_ast, tables);
  auto subquery_tbl = subquery.asInstanceOf<TableExpressionNode>();
  auto resolver = [&subquery_tbl, &subquery_alias] (const String& name) {
    auto col = name;
    if (!subquery_alias.empty()) {
      if (StringUtil::beginsWith(col, subquery_alias + ".")) {
        col = col.substr(subquery_alias.size() + 1);
      }
    }

    return subquery_tbl->getColumnIndex(col);
  };

  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : subquery_tbl->outputColumns()) {
        auto sl = new SelectListNode(new ColumnReferenceNode(col));
        sl->setAlias(col);
        QueryTreeUtil::resolveColumns(sl->expression(), resolver);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      auto sl = buildSelectList(txn, select_expr);
      QueryTreeUtil::resolveColumns(sl->expression(), resolver);
      select_list_expressions.emplace_back(sl);
    }
  }

  /* get where expression */
  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (!in_join && where_clause) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];

    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    where_expr = Some(RefPtr<ValueExpressionNode>(buildValueExpression(txn, e)));
    QueryTreeUtil::resolveColumns(where_expr.get(), resolver);
  }

  auto subqnode = new SubqueryNode(
      subquery,
      select_list_expressions,
      where_expr);
  subqnode->setTableAlias(subquery_alias);

  return subqnode;
}

QueryTreeNode* QueryPlanBuilder::buildSeqscanTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (!(*table_ref == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (table_ref->getChildren().size() < 1) {
    return nullptr;
  }

  /* get table reference */
  auto tbl_name = table_ref->getChildren()[0];
  if (!(*tbl_name == ASTNode::T_TABLE_NAME)) {
    return nullptr;
  }

  auto tbl_name_token = tbl_name->getToken();
  if (!(tbl_name_token != nullptr)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto table_name = tbl_name_token->getString();

  /* get table alias */
  String table_alias;
  if (table_ref->getChildren().size() > 1 &&
      table_ref->getChildren()[1]->getType() == ASTNode::T_TABLE_ALIAS) {
    table_alias = table_ref->getChildren()[1]->getToken()->getString();
  }

  auto table = tables->describe(table_name);
  if (table.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: '$0'", table_name);
  }

  /* get where expression */
  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (where_clause) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];

    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    auto pred = RefPtr<ValueExpressionNode>(buildValueExpression(txn, e));

    if (in_join) {
      Set<String> valid_columns;
      for (const auto& col : table.get().columns) {
        valid_columns.insert(col.column_name);
        valid_columns.insert(table_name + "." + col.column_name);
        if (!table_alias.empty()) {
          valid_columns.insert(table_alias + "." + col.column_name);
        }
      }

      pred = QueryTreeUtil::prunePredicateExpression(pred, valid_columns);
    }

    // FIXME skip if literal true expression
    where_expr = Some(pred);
  }

  bool has_aggregation = false;
  bool has_aggregation_within_record = false;

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : table.get().columns) {
        auto sl = new SelectListNode(new ColumnReferenceNode(col.column_name));
        sl->setAlias(col.column_name);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      if (hasAggregationExpression(select_expr)) {
        has_aggregation = true;
      }

      if (hasAggregationWithinRecord(select_expr)) {
        has_aggregation_within_record = true;
      }

      select_list_expressions.emplace_back(buildSelectList(txn, select_expr));
    }
  }

  if (has_aggregation && has_aggregation_within_record) {
    RAISE(
        kRuntimeError,
        "invalid use of aggregation WITHIN RECORD functions");
  }


  /* aggregation type */
  auto seqscan = new SequentialScanNode(
      table.get(),
      tables,
      select_list_expressions,
      where_expr);

  if (!table_alias.empty()) {
    seqscan->setTableAlias(table_alias);
  }

  if (has_aggregation) {
    seqscan->setAggregationStrategy(AggregationStrategy::AGGREGATE_ALL);
  }

  if (has_aggregation_within_record) {
    seqscan->setAggregationStrategy(AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT);
  }

  seqscan->normalizeColumnNames();
  return seqscan;
}

RefPtr<ValueExpressionNode> QueryPlanBuilder::buildValueExpression(
    Transaction* txn,
    ASTNode* ast) {
  auto valexpr = buildUnoptimizedValueExpression(txn, ast);

  if (opts_.enable_constant_folding) {
    valexpr = QueryTreeUtil::foldConstants(txn, valexpr);
  }

  return valexpr;
}

RefPtr<ValueExpressionNode> QueryPlanBuilder::buildUnoptimizedValueExpression(
    Transaction* txn,
    ASTNode* ast) {
  if (ast == nullptr) {
    RAISE(kNullPointerError, "can't build nullptr");
  }

  switch (ast->getType()) {

    case ASTNode::T_EQ_EXPR:
      return buildOperator(txn, "eq", ast);

    case ASTNode::T_NEQ_EXPR:
      return buildOperator(txn, "neq", ast);

    case ASTNode::T_AND_EXPR:
      return buildOperator(txn, "logical_and", ast);

    case ASTNode::T_OR_EXPR:
      return buildOperator(txn, "logical_or", ast);

    case ASTNode::T_NEGATE_EXPR:
      return buildOperator(txn, "neg", ast);

    case ASTNode::T_LT_EXPR:
      return buildOperator(txn, "lt", ast);

    case ASTNode::T_LTE_EXPR:
      return buildOperator(txn, "lte", ast);

    case ASTNode::T_GT_EXPR:
      return buildOperator(txn, "gt", ast);

    case ASTNode::T_GTE_EXPR:
      return buildOperator(txn, "gte", ast);

    case ASTNode::T_ADD_EXPR:
      return buildOperator(txn, "add", ast);

    case ASTNode::T_SUB_EXPR:
      return buildOperator(txn, "sub", ast);

    case ASTNode::T_MUL_EXPR:
      return buildOperator(txn, "mul", ast);

    case ASTNode::T_DIV_EXPR:
      return buildOperator(txn, "div", ast);

    case ASTNode::T_MOD_EXPR:
      return buildOperator(txn, "mod", ast);

    case ASTNode::T_POW_EXPR:
      return buildOperator(txn, "pow", ast);

    case ASTNode::T_REGEX_EXPR:
      return buildRegex(txn, ast);

    case ASTNode::T_LIKE_EXPR:
      return buildLike(txn, ast);

    case ASTNode::T_LITERAL:
      return buildLiteral(txn, ast);

    case ASTNode::T_VOID:
      return new LiteralExpressionNode(SValue("void"));

    case ASTNode::T_IF_EXPR:
      return buildIfStatement(txn, ast);

    case ASTNode::T_RESOLVED_COLUMN:
    case ASTNode::T_COLUMN_NAME:
      return buildColumnReference(txn, ast);

    case ASTNode::T_COLUMN_INDEX:
      return buildColumnIndex(txn, ast);

    case ASTNode::T_TABLE_NAME:
      return buildColumnReference(txn, ast->getChildren()[0]);

    case ASTNode::T_METHOD_CALL:
    case ASTNode::T_METHOD_CALL_WITHIN_RECORD:
      return buildMethodCall(txn, ast);

    default:
      ast->debugPrint();
      RAISE(kRuntimeError, "internal error: can't build expression");
  }
}

ValueExpressionNode* QueryPlanBuilder::buildLiteral(
    Transaction* txn,
    ASTNode* ast) {
  if (ast->getToken() == nullptr) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  SValue literal;
  auto token = ast->getToken();

  switch (token->getType()) {

    case Token::T_TRUE:
      literal = SValue(true);
      break;

    case Token::T_FALSE:
      literal = SValue(false);
      break;

    case Token::T_NUMERIC:
      literal = SValue(token->getString()).toNumeric();
      break;

    case Token::T_STRING:
      literal = SValue(token->getString());
      break;

    case Token::T_NULL:
      literal = SValue();
      break;

    default:
      RAISE(kRuntimeError, "can't cast Token to SValue");

  }

  return new LiteralExpressionNode(literal);
}

ValueExpressionNode* QueryPlanBuilder::buildOperator(
    Transaction* txn,
    const std::string& name,
    ASTNode* ast) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e));
  }

  return new CallExpressionNode(name, args);
}

ValueExpressionNode* QueryPlanBuilder::buildMethodCall(
    Transaction* txn,
    ASTNode* ast) {
  if (ast->getToken() == nullptr ||
      ast->getToken()->getType() != Token::T_IDENTIFIER) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto symbol = ast->getToken()->getString();

  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e));
  }

  return new CallExpressionNode(symbol, args);
}

ValueExpressionNode* QueryPlanBuilder::buildIfStatement(
    Transaction* txn,
    ASTNode* ast) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e));
  }

  if (args.size() != 3) {
    RAISE(kRuntimeError, "if statement must have exactly 3 arguments");
  }

  return new IfExpressionNode(args[0], args[1], args[2]);
}

ValueExpressionNode* QueryPlanBuilder::buildColumnReference(
    Transaction* txn,
    ASTNode* ast) {
  String column_name;

  for (
      auto cur = ast;
      cur->getToken() != nullptr &&
      cur->getToken()->getType() == Token::T_IDENTIFIER; ) {
    if (cur != ast) column_name += ".";
    column_name += cur->getToken()->getString();

    if (cur->getChildren().size() != 1) {
      break;
    } else {
      cur = cur->getChildren()[0];
    }
  }

  auto colref = new ColumnReferenceNode(column_name);
  colref->setColumnIndex(ast->getID());
  return colref;
}

ValueExpressionNode* QueryPlanBuilder::buildColumnIndex(
    Transaction* txn,
    ASTNode* ast) {
  if (ast->getChildren().size() != 1) {
    RAISE(kRuntimeError, "internal error: invalid column index reference");
  }

  auto token = ast->getChildren()[0]->getToken();
  if (!token || token->getType() != Token::T_NUMERIC) {
    RAISE(kRuntimeError, "internal error: invalid column index reference");
  }

  return new ColumnReferenceNode(token->getInteger());
}

ValueExpressionNode* QueryPlanBuilder::buildRegex(
    Transaction* txn,
    ASTNode* ast) {
  const auto& args = ast->getChildren();
  if (args.size() != 2) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  if (args[1]->getType() != ASTNode::T_LITERAL ||
      args[1]->getToken() == nullptr ||
      args[1]->getToken()->getType() != Token::T_STRING) {
    RAISE(
        kRuntimeError,
        "second argument to REGEX operator must be a string literal");
  }

  auto pattern = args[1]->getToken()->getString();
  auto subject = buildValueExpression(txn, args[0]);

  return new RegexExpressionNode(subject, pattern);
}

ValueExpressionNode* QueryPlanBuilder::buildLike(
    Transaction* txn,
    ASTNode* ast) {
  const auto& args = ast->getChildren();
  if (args.size() != 2) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  if (args[1]->getType() != ASTNode::T_LITERAL ||
      args[1]->getToken() == nullptr ||
      args[1]->getToken()->getType() != Token::T_STRING) {
    RAISE(
        kRuntimeError,
        "second argument to LIKE operator must be a string literal");
  }

  auto pattern = args[1]->getToken()->getString();
  auto subject = buildValueExpression(txn, args[0]);

  return new LikeExpressionNode(subject, pattern);
}

SelectListNode* QueryPlanBuilder::buildSelectList(
    Transaction* txn,
    ASTNode* ast) {
  if (ast->getChildren().size() == 0) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  auto slnode = new SelectListNode(
      buildValueExpression(txn, ast->getChildren()[0]));

  /* .. AS alias */
  if (ast->getType() == ASTNode::T_DERIVED_COLUMN &&
      ast->getChildren().size() > 1 &&
      ast->getChildren()[1]->getType() == ASTNode::T_COLUMN_ALIAS) {
    slnode->setAlias(ast->getChildren()[1]->getToken()->getString());
  };

  return slnode;
}

QueryTreeNode* QueryPlanBuilder::buildShowTables(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_SHOW_TABLES)) {
    return nullptr;
  }

  return new ShowTablesNode();
}

QueryTreeNode* QueryPlanBuilder::buildDescribeTable(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_DESCRIBE_TABLE) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new DescribeTableNode(table_name->getToken()->getString());
}

}
