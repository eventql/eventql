/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/parser/astnode.h>
#include <csql/parser/astutil.h>
#include <csql/parser/token.h>
#include <csql/runtime/tablerepository.h>
#include <stx/exception.h>

namespace csql {

std::vector<std::string> ASTUtil::columnNamesFromSelectList(
    ASTNode* select_list,
    TableRef* tbl_ref /* = nullptr */) {
  std::vector<std::string> column_names;
  for (auto col : select_list->getChildren()) {
    if (col->getType() != ASTNode::T_DERIVED_COLUMN) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto derived = col->getChildren();

    // column with AS clause
    if (derived.size() == 2) {
      if (derived[1]->getType() != ASTNode::T_COLUMN_ALIAS) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto colname_token = derived[1]->getToken();

      if (!(colname_token && *colname_token == Token::T_IDENTIFIER)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      column_names.emplace_back(colname_token->getString());
      continue;
    }

    // resolved column name
    if (derived.size() == 1 && *derived[0] == ASTNode::T_RESOLVED_COLUMN) {
      if (tbl_ref == nullptr) {
        column_names.emplace_back("col" + std::to_string(derived[0]->getID()));
      } else {
        auto col_name = tbl_ref->getColumnName(derived[0]->getID());
        column_names.emplace_back(col_name);
      }

      continue;
    }

    // expression
    column_names.emplace_back("<expr>"); // FIXPAUL!!
  }

  return column_names;
}

String ASTUtil::columnNameForExpression(ASTNode* expr) {
  switch (expr->getType()) {

    case ASTNode::T_LITERAL:
      return expr->getToken()->getString();

    case ASTNode::T_COLUMN_NAME:
    case ASTNode::T_RESOLVED_COLUMN: {
      auto str = expr->getToken()->getString();
      for (const auto& c : expr->getChildren()) {
        str += "." + columnNameForExpression(c);
      }

      return str;
    }

    case ASTNode::T_RESOLVED_CALL:
    case ASTNode::T_METHOD_CALL: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format(
          "$0($1)",
          expr->getToken()->getString(),
          StringUtil::join(args, ", "));
    }

    case ASTNode::T_METHOD_CALL_WITHIN_RECORD: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format(
          "$0($1) WITHIN RECORD",
          expr->getToken()->getString(),
          StringUtil::join(args, ", "));
    }

    case ASTNode::T_IF_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("if($0)", StringUtil::join(args, ", "));
    }

    case ASTNode::T_EQ_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " == "));
    }

    case ASTNode::T_NEQ_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " != "));
    }

    case ASTNode::T_LT_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " < "));
    }

    case ASTNode::T_LTE_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " <= "));
    }

    case ASTNode::T_GT_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " > "));
    }

    case ASTNode::T_GTE_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " >= "));
    }

    case ASTNode::T_AND_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " AND "));
    }

    case ASTNode::T_OR_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " OR "));
    }

    case ASTNode::T_NEGATE_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("!($0)", StringUtil::join(args, ", "));
    }

    case ASTNode::T_ADD_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " + "));
    }

    case ASTNode::T_SUB_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " - "));
    }

    case ASTNode::T_MUL_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " * "));
    }

    case ASTNode::T_DIV_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " / "));
    }

    case ASTNode::T_MOD_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " % "));
    }

    case ASTNode::T_POW_EXPR: {
      Vector<String> args;
      for (const auto& c : expr->getChildren()) {
        args.emplace_back(columnNameForExpression(c));
      }

      return StringUtil::format("($0)", StringUtil::join(args, " ^ "));
    }

    default:
      return "<expr>";

  }
}

}
