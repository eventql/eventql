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
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/parser/astutil.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/util/exception.h>

namespace csql {

String ASTUtil::columnNameForExpression(ASTNode* expr) {
  switch (expr->getType()) {

    case ASTNode::T_LITERAL:
      return expr->getToken()->getString();

    case ASTNode::T_COLUMN_NAME:
    case ASTNode::T_TABLE_NAME:
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
