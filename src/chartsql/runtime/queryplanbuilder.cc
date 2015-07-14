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
#include <chartsql/parser/astnode.h>
#include <chartsql/parser/astutil.h>
#include <chartsql/parser/parser.h>
#include <chartsql/runtime/queryplanbuilder.h>
#include <chartsql/qtree/GroupByNode.h>
#include <chartsql/qtree/IfExpressionNode.h>
#include <chartsql/qtree/SelectExpressionNode.h>
#include <chartsql/qtree/LimitNode.h>
#include <chartsql/qtree/OrderByNode.h>
#include <chartsql/qtree/DrawStatementNode.h>
#include <chartsql/qtree/ChartStatementNode.h>
#include <chartsql/qtree/ShowTablesNode.h>
#include <chartsql/qtree/DescribeTableNode.h>

namespace csql {

QueryPlanBuilder::QueryPlanBuilder(
    SymbolTable* symbol_table) :
    symbol_table_(symbol_table) {}

RefPtr<QueryTreeNode> QueryPlanBuilder::build(
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  QueryTreeNode* node = nullptr;

  /* expand all column names + wildcard to tablename->columnanme */
  if (hasUnexpandedColumns(ast)) {
    expandColumns(ast, tables);
  }

  /* internal nodes: multi table query (joins), order, aggregation, limit */
  if ((node = buildLimitClause(ast, tables)) != nullptr) {
    return node;
  }

  if (hasOrderByClause(ast)) {
    return buildOrderByClause(ast, tables);
  }

//  if (hasGroupOverTimewindowClause(ast)) {
//    return buildGroupOverTimewindow(ast, repo);
//  }

  if (hasGroupByClause(ast) || hasAggregationInSelectList(ast)) {
    return buildGroupBy(ast, tables);
  }

  /* leaf nodes: table scan, tableless select */
  if ((node = buildSequentialScan(ast)) != nullptr) {
    return node;
  }

  if ((node = buildSelectExpression(ast)) != nullptr) {
    return node;
  }

  /* other statments */
  if ((node = buildShowTables(ast)) != nullptr) {
    return node;
  }

  if ((node = buildDescribeTable(ast)) != nullptr) {
    return node;
  }

  ast->debugPrint(2);
  RAISE(kRuntimeError, "can't figure out a query plan for this, sorry :(");
}

Vector<RefPtr<QueryTreeNode>> QueryPlanBuilder::build(
    const Vector<ASTNode*>& statements,
    RefPtr<TableProvider> tables) {
  Vector<RefPtr<QueryTreeNode>> nodes;

  for (int i = 0; i < statements.size(); ++i) {
    switch (statements[i]->getType()) {

      case ASTNode::T_SELECT:
      case ASTNode::T_SHOW_TABLES:
      case ASTNode::T_DESCRIBE_TABLE:
        nodes.emplace_back(build(statements[i], tables));
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
                subselects.emplace_back(build(statements[i++], tables));
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

bool QueryPlanBuilder::hasUnexpandedColumns(ASTNode* ast) const {
  if (ast->getType() != ASTNode::T_SELECT) {
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
    if (col->getType() != ASTNode::T_DERIVED_COLUMN ||
        col->getChildren().size() != 1 ||
        col->getChildren()[0]->getType() != ASTNode::T_TABLE_NAME ||
        col->getChildren()[0]->getChildren().size() != 1 ||
        col->getChildren()[0]->getChildren()[0]->getType() !=
        ASTNode::T_COLUMN_NAME) {
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
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
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
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
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

void QueryPlanBuilder::expandColumns(
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (ast->getChildren().size() < 2) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0];
  if (select_list->getType() != ASTNode::T_SELECT_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto from_list = ast->getChildren()[1];
  if (from_list->getType() != ASTNode::T_FROM ||
      from_list->getChildren().size() < 1) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  for (const auto& node : select_list->getChildren()) {
    /* expand wildcard select (SELECT * FROM ...) */
    if (node->getType() == ASTNode::T_ALL) {
      if (hasJoin(ast) || from_list->getChildren().size() != 1) {
        RAISE(
            kRuntimeError,
            "can't use wilcard select (SELECT * FROM ...) when selecting from "
            "multiple tables");
      }

      auto table_name = from_list->getChildren()[0];
      if (table_name->getType() != ASTNode::T_TABLE_NAME ||
          table_name->getToken() == nullptr) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      select_list->removeChild(node);

      auto tbl_info = tables->describe(table_name->getToken()->getString());
      if (tbl_info.isEmpty()) {
        RAISEF(
            kNotFoundError,
            "table not found: $)",
            table_name->getToken()->getString());
      }

      for (const auto& column : tbl_info.get().columns) {
        auto derived_col = new ASTNode(ASTNode::T_DERIVED_COLUMN);
        auto derived_table_name = new ASTNode(ASTNode::T_TABLE_NAME);
        derived_table_name->setToken(table_name->getToken());
        auto column_name = new ASTNode(ASTNode::T_COLUMN_NAME);
        column_name->setToken(new Token(Token::T_IDENTIFIER, column.column_name));
        derived_col->appendChild(derived_table_name);
        derived_table_name->appendChild(column_name);
        select_list->appendChild(derived_col);
      }

      continue;
    }
  }

}


QueryTreeNode* QueryPlanBuilder::buildGroupBy(
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  /* copy own select list */
  if (!(ast->getChildren()[0]->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0]->deepCopy();

  /* generate select list for child */
  auto child_sl = new ASTNode(ASTNode::T_SELECT_LIST);
  buildInternalSelectList(select_list, child_sl);

  /* copy ast for child and swap out select lists*/
  auto child_ast = ast->deepCopy();
  child_ast->removeChildByIndex(0);
  child_ast->appendChild(child_sl, 0);

  /* search for a group by clause */
  Vector<RefPtr<ValueExpressionNode>> group_expressions;
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_GROUP_BY) {
      continue;
    }

    /* FIXPAUL resolve aliases in group list from select list, return error
       if alias contains aggregate func */

    /* copy all group expressions and add required field to child select list */
    for (const auto& group_expr : child->getChildren()) {
      auto e = group_expr->deepCopy();
      buildInternalSelectList(e, child_sl);

      if (hasAggregationExpression(e)) {
        RAISE(kRuntimeError, "GROUP clause can only contain pure functions");
      }

      group_expressions.emplace_back(buildValueExpression(e));
    }

    /* remove group by clause from child ast */
    child_ast->removeChildrenByType(ASTNode::T_GROUP_BY);
  }

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    select_list_expressions.emplace_back(buildSelectList(select_expr));
  }

  return new GroupByNode(
      select_list_expressions,
      group_expressions,
      build(child_ast, tables));
}

//QueryPlanNode* QueryPlanBuilder::buildGroupOverTimewindow(
//    ASTNode* ast,
//    TableRepository* repo) {
//  ASTNode group_exprs(ASTNode::T_GROUP_BY);
//  ASTNode* time_expr_ast;
//  ASTNode* window_expr_ast;
//  ASTNode* step_expr_ast = nullptr;
//
//  /* copy own select list */
//  if (!(ast->getChildren()[0]->getType() == ASTNode::T_SELECT_LIST)) {
//    RAISE(kRuntimeError, "corrupt AST");
//  }
//
//  auto select_list = ast->getChildren()[0]->deepCopy();
//
//  /* generate select list for child */
//  auto child_sl = new ASTNode(ASTNode::T_SELECT_LIST);
//  buildInternalSelectList(select_list, child_sl);
//
//  /* copy ast for child and swap out select lists*/
//  auto child_ast = ast->deepCopy();
//  child_ast->removeChildByIndex(0);
//  child_ast->appendChild(child_sl, 0);
//
//  /* search for a group over timewindow clause */
//  for (const auto& child : ast->getChildren()) {
//    if (child->getType() != ASTNode::T_GROUP_OVER_TIMEWINDOW) {
//      continue;
//    }
//
//    if (child->getChildren().size() < 3) {
//      RAISE(kRuntimeError, "corrupt AST");
//    }
//
//    /* FIXPAUL resolve aliases in group list from select list, return error
//       if alias contains aggregate func */
//
//    /* copy time expression and add required fields to the child select list */
//    time_expr_ast = child->getChildren()[0]->deepCopy();
//    buildInternalSelectList(time_expr_ast, child_sl);
//
//    /* copy all group exprs and add required fields to the child select list */
//    auto group_by_list = child->getChildren()[1];
//    for (const auto& group_expr : group_by_list->getChildren()) {
//      auto e = group_expr->deepCopy();
//      buildInternalSelectList(e, child_sl);
//      group_exprs.appendChild(e);
//    }
//
//    /* copy window and step expressions */
//    window_expr_ast = child->getChildren()[2]->deepCopy();
//    if (child->getChildren().size() > 3) {
//      step_expr_ast = child->getChildren()[3]->deepCopy();
//    }
//
//    /* remove group by clause from child ast */
//    child_ast->removeChildrenByType(ASTNode::T_GROUP_OVER_TIMEWINDOW);
//  }
//
//  /* compile select list and group expressions */
//  size_t select_scratchpad_len = 0;
//  auto select_expr = compiler_->compile(select_list, &select_scratchpad_len);
//
//  size_t group_scratchpad_len = 0;
//  auto group_expr = compiler_->compile(&group_exprs, &group_scratchpad_len);
//
//  if (group_scratchpad_len > 0) {
//    RAISE(
//        kRuntimeError,
//        "illegal use of aggregate functions in the GROUP BY clause");
//  }
//
//  /* compile time expression */
//  size_t time_expr_scratchpad_len = 0;
//  auto time_expr = compiler_->compile(
//      time_expr_ast,
//      &time_expr_scratchpad_len);
//
//  if (time_expr_scratchpad_len > 0) {
//    RAISE(
//        kRuntimeError,
//        "illegal use of aggregate functions in the GROUP OVER clause");
//  }
//
//  /* find time expression input column */
//  if (time_expr_ast->getType() != ASTNode::T_RESOLVED_COLUMN) {
//    RAISE(
//        kRuntimeError,
//        "first argument to TIMEWINDOW() must be a column reference");
//  }
//
//  auto input_row_size = child_sl->getChildren().size();
//  auto input_row_time_index = time_expr_ast->getID();
//
//  /* compile window and step */
//  auto window_svalue = executeSimpleConstExpression(compiler_, window_expr_ast);
//  auto window = window_svalue.getInteger();
//
//  SValue::IntegerType step;
//  if (step_expr_ast == nullptr) {
//    step = window;
//  } else {
//    auto step_svalue = executeSimpleConstExpression(compiler_, step_expr_ast);
//    step = step_svalue.getInteger();
//  }
//
//  /* resolve output column names */
//  auto column_names = ASTUtil::columnNamesFromSelectList(select_list);
//
//  return new GroupOverTimewindow(
//      std::move(column_names),
//      time_expr,
//      window,
//      step,
//      input_row_size,
//      input_row_time_index,
//      select_expr,
//      group_expr,
//      select_scratchpad_len,
//      buildQueryPlan(child_ast, repo));
//}
//
bool QueryPlanBuilder::buildInternalSelectList(
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
      return true;
    }

    /* push down referenced columns into the child select list */
    case ASTNode::T_COLUMN_NAME: {
      auto col_index = -1;

      /* check if this column already exists in the select list */
      const auto& candidates = target_select_list->getChildren();
      for (int i = 0; i < candidates.size(); ++i) {
        if (candidates[i]->getType() == ASTNode::T_DERIVED_COLUMN) {
          if (candidates[i]->getChildren().size() == 1) {
            auto colname = candidates[i]->getChildren()[0];
            if (colname->getType() == ASTNode::T_COLUMN_NAME &&
                node->getToken()->getString() ==
                    colname->getToken()->getString()) {
              col_index = i;
              break;
            }
          }
        }
      }

      /* otherwise add this column to the select list */
      if (col_index < 0) {
        auto derived = new ASTNode(ASTNode::T_DERIVED_COLUMN);
        derived->appendChild(node->deepCopy());
        target_select_list->appendChild(derived);
        col_index = target_select_list->getChildren().size() - 1;
      }

      node->setType(ASTNode::T_RESOLVED_COLUMN);
      node->setID(col_index);
      return true;
    }

    default: {
      for (const auto& child : node->getChildren()) {
        if (!buildInternalSelectList(child, target_select_list)) {
          return false;
        }
      }

      return true;
    }

  }
}

QueryTreeNode* QueryPlanBuilder::buildLimitClause(
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 3) {
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
        build(new_ast, tables));
  }

  return nullptr;
}

QueryTreeNode* QueryPlanBuilder::buildOrderByClause(
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  Vector<OrderByNode::SortSpec> sort_specs;

  /* copy select list for child */
  if (!(ast->getChildren()[0]->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto child_sl = ast->getChildren()[0]->deepCopy();

  /* search for the order by clause */
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_ORDER_BY) {
      continue;
    }

    /* build each sort spec and expand select list for missing columns */
    auto sort_specs_asts = child->getChildren();
    for (int i = 0; i < sort_specs_asts.size(); ++i) {
      auto sort = sort_specs_asts[i];
      auto col = sort->getChildren()[0];
      size_t col_index;
      bool col_found = false;

      /* check if sort spec is referencing a column alias from the select list */
      auto child_sllist = child_sl->getChildren();
      for (int n = 0; n < child_sllist.size(); ++n) {
        const auto& derived = child_sllist[n];

        if (derived->getType() == ASTNode::T_DERIVED_COLUMN &&
            derived->getChildren().size() > 1 &&
            derived->getChildren()[1]->getType() == ASTNode::T_COLUMN_ALIAS) {

          auto this_alias = derived->getChildren()[1]->getToken()->getString();
          if (this_alias == col->getToken()->getString()) { // FIXPAUL case insensitive match
            col_index = n;
            col_found = true;
            break;
          }
        }
      }

      /* otherwise add the column to the child select list */
      if (!col_found) {
        auto new_derived = new ASTNode(ASTNode::T_DERIVED_COLUMN);
        new_derived->appendChild(col);
        child_sl->appendChild(new_derived);
        col_index = child_sl->getChildren().size() - 1;
      }

      auto sort_descending = sort->getToken() != nullptr &&
          sort->getToken()->getType() == Token::T_DESC;

      /* create the sort spec */
      OrderByNode::SortSpec sort_spec;
      sort_spec.column = col_index;
      sort_spec.descending = sort_descending;
      sort_specs.emplace_back(sort_spec);
    }
  }

  /* copy ast for child and swap out select lists, remove order by clause */
  auto child_ast = ast->deepCopy();
  child_ast->removeChildByIndex(0);
  child_ast->appendChild(child_sl, 0);
  child_ast->removeChildrenByType(ASTNode::T_ORDER_BY);

  return new OrderByNode(
      sort_specs,
      ast->getChildren()[0]->getChildren().size(),
      build(child_ast, tables));
}

QueryTreeNode* QueryPlanBuilder::buildSequentialScan(ASTNode* ast) {
  if (!(*ast == ASTNode::T_SELECT)) {
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

  if (from_list->getChildren().size() != 1) {
    return nullptr;
  }

  /* get table reference */
  auto tbl_name = from_list->getChildren()[0];

  if (!(*tbl_name == ASTNode::T_TABLE_NAME)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto tbl_name_token = tbl_name->getToken();
  if (!(tbl_name_token != nullptr)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto table_name = tbl_name_token->getString();

  /* get select list */
  if (!(*ast->getChildren()[0] == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto select_list = ast->getChildren()[0];

  /* resolve column references and compile ast */
  if (select_list == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  /* get where expression */
  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (ast->getChildren().size() > 2) {
    ASTNode* where_clause = ast->getChildren()[2];
    if (!(where_clause)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

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

    where_expr = Some(RefPtr<ValueExpressionNode>(buildValueExpression(e)));
  }

  bool has_aggregation = false;
  bool has_aggregation_within_record = false;

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (hasAggregationExpression(select_expr)) {
      has_aggregation = true;
    }

    if (hasAggregationWithinRecord(select_expr)) {
      has_aggregation_within_record = true;
    }

    select_list_expressions.emplace_back(buildSelectList(select_expr));
  }

  if (has_aggregation && has_aggregation_within_record) {
    RAISE(
        kRuntimeError,
        "invalid use of aggregation WITHIN RECORD functions");
  }

  /* aggregation type */
  auto seqscan = new SequentialScanNode(
      table_name,
      select_list_expressions,
      where_expr);

  if (has_aggregation) {
    seqscan->setAggregationStrategy(AggregationStrategy::AGGREGATE_ALL);
  }

  if (has_aggregation_within_record) {
    seqscan->setAggregationStrategy(
        AggregationStrategy::AGGREGATE_WITHIN_RECORD);
  }

  return seqscan;
}

QueryTreeNode* QueryPlanBuilder::buildSelectExpression(ASTNode* ast) {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto select_list = ast->getChildren()[0];

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (hasAggregationExpression(select_expr) ||
        hasAggregationWithinRecord(select_expr)) {
      RAISE(
          kRuntimeError,
          "a SELECT without any tables can only contain pure functions");
    }

    select_list_expressions.emplace_back(buildSelectList(select_expr));
  }

  return new SelectExpressionNode(select_list_expressions);
}

ValueExpressionNode* QueryPlanBuilder::buildValueExpression(ASTNode* ast) {
  if (ast == nullptr) {
    RAISE(kNullPointerError, "can't build nullptr");
  }

  switch (ast->getType()) {

    case ASTNode::T_EQ_EXPR:
      return buildOperator("eq", ast);

    case ASTNode::T_NEQ_EXPR:
      return buildOperator("neq", ast);

    case ASTNode::T_AND_EXPR:
      return buildOperator("and", ast);

    case ASTNode::T_OR_EXPR:
      return buildOperator("or", ast);

    case ASTNode::T_NEGATE_EXPR:
      return buildOperator("neg", ast);

    case ASTNode::T_LT_EXPR:
      return buildOperator("lt", ast);

    case ASTNode::T_LTE_EXPR:
      return buildOperator("lte", ast);

    case ASTNode::T_GT_EXPR:
      return buildOperator("gt", ast);

    case ASTNode::T_GTE_EXPR:
      return buildOperator("gte", ast);

    case ASTNode::T_ADD_EXPR:
      return buildOperator("add", ast);

    case ASTNode::T_SUB_EXPR:
      return buildOperator("sub", ast);

    case ASTNode::T_MUL_EXPR:
      return buildOperator("mul", ast);

    case ASTNode::T_DIV_EXPR:
      return buildOperator("div", ast);

    case ASTNode::T_MOD_EXPR:
      return buildOperator("mod", ast);

    case ASTNode::T_POW_EXPR:
      return buildOperator("pow", ast);

    case ASTNode::T_LITERAL:
      return buildLiteral(ast);

    case ASTNode::T_IF_EXPR:
      return buildIfStatement(ast);

    case ASTNode::T_RESOLVED_COLUMN:
    case ASTNode::T_COLUMN_NAME:
      return buildColumnReference(ast);

    case ASTNode::T_METHOD_CALL:
    case ASTNode::T_METHOD_CALL_WITHIN_RECORD:
      return buildMethodCall(ast);

    default:
      ast->debugPrint();
      RAISE(kRuntimeError, "internal error: can't build expression");
  }
}

ValueExpressionNode* QueryPlanBuilder::buildLiteral(ASTNode* ast) {
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
      literal = SValue(token->getString());
      literal.tryNumericConversion();
      break;

    case Token::T_STRING:
      literal = SValue(token->getString());
      break;

    default:
      RAISE(kRuntimeError, "can't cast Token to SValue");

  }

  return new LiteralExpressionNode(literal);
}

ValueExpressionNode* QueryPlanBuilder::buildOperator(
    const std::string& name,
    ASTNode* ast) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(e));
  }

  return new CallExpressionNode(name, args);
}

ValueExpressionNode* QueryPlanBuilder::buildMethodCall(ASTNode* ast) {
  if (ast->getToken() == nullptr ||
      ast->getToken()->getType() != Token::T_IDENTIFIER) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto symbol = ast->getToken()->getString();

  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(e));
  }

  return new CallExpressionNode(symbol, args);
}

ValueExpressionNode* QueryPlanBuilder::buildIfStatement(ASTNode* ast) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(e));
  }

  if (args.size() != 3) {
    RAISE(kRuntimeError, "if statement must have exactly 3 arguments");
  }

  return new IfExpressionNode(args[0], args[1], args[2]);
}

ValueExpressionNode* QueryPlanBuilder::buildColumnReference(ASTNode* ast) {
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

SelectListNode* QueryPlanBuilder::buildSelectList(ASTNode* ast) {
  if (ast->getChildren().size() == 0) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  auto slnode = new SelectListNode(buildValueExpression(ast->getChildren()[0]));

  /* .. AS alias */
  if (ast->getType() == ASTNode::T_DERIVED_COLUMN &&
      ast->getChildren().size() > 1 &&
      ast->getChildren()[1]->getType() == ASTNode::T_COLUMN_ALIAS) {
    slnode->setAlias(ast->getChildren()[1]->getToken()->getString());
  };

  return slnode;
}

QueryTreeNode* QueryPlanBuilder::buildShowTables(ASTNode* ast) {
  if (!(*ast == ASTNode::T_SHOW_TABLES)) {
    return nullptr;
  }

  return new ShowTablesNode();
}

QueryTreeNode* QueryPlanBuilder::buildDescribeTable(ASTNode* ast) {
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
