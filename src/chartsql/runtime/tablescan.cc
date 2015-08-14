 /**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/parser/astutil.h>
#include <chartsql/runtime/tablescan.h>

namespace csql {

TableScan::TableScan(
    TableRef* tbl_ref,
    std::vector<std::string>&& columns,
    VM::Instruction* select_expr,
    VM::Instruction* where_expr):
    tbl_ref_(tbl_ref),
    columns_(std::move(columns)),
    select_expr_(select_expr),
    where_expr_(where_expr) {}

void TableScan::execute() {
  tbl_ref_->executeScan(this);
  finish();
}

bool TableScan::nextRow(SValue* row, int row_len) {
  RAISE(kNotImplementedError);
  //auto pred_bool = true;
  //auto continue_bool = true;

  //SValue out[128]; // FIXPAUL
  //int out_len;

  //if (where_expr_ != nullptr) {
  //  executeExpression(where_expr_, nullptr, row_len, row, &out_len, out);

  //  if (out_len != 1) {
  //    RAISE(
  //        kRuntimeError,
  //        "WHERE predicate expression evaluation did not return a result");
  //  }

  //  pred_bool = out[0].getBool();
  //}

  //if (pred_bool) {
  //  executeExpression(select_expr_, nullptr, row_len, row, &out_len, out);
  //  continue_bool = emitRow(out, out_len);
  //}

  //return continue_bool;
}

size_t TableScan::getNumCols() const {
  return columns_.size();
}

const std::vector<std::string>& TableScan::getColumns() const {
  return columns_;
}

/* recursively walk the ast and resolve column references */
bool TableScan::resolveColumns(
    ASTNode* node,
    ASTNode* parent,
    TableRef* tbl_ref) {
  if (node == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  switch (node->getType()) {
    case ASTNode::T_TABLE_NAME: {
      if (node->getChildren().size() != 1) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto column_name = node->getChildren()[0];
      if (column_name->getType() != ASTNode::T_COLUMN_NAME) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto token = column_name->getToken();
      if (!(token && *token == Token::T_IDENTIFIER)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto col_index = tbl_ref->getColumnIndex(token->getString());
      if (col_index < 0) {
        RAISE(
            kRuntimeError,
            "no such column: '%s'",
            token->getString().c_str());
        return false;
      }

      node->setType(ASTNode::T_RESOLVED_COLUMN);
      node->setID(col_index);
      return true;
    }

    case ASTNode::T_COLUMN_NAME: {
      auto token = node->getToken();
      if (!(token && *token == Token::T_IDENTIFIER)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto col_index = tbl_ref->getColumnIndex(token->getString());
      if (col_index < 0) {
        RAISE(
            kRuntimeError,
            "no such column: '%s'",
            token->getString().c_str());
        return false;
      }

      node->setType(ASTNode::T_RESOLVED_COLUMN);
      node->setID(col_index);
      return true;
    }

    default: {
      for (const auto& child : node->getChildren()) {
        if (child == nullptr) {
          RAISE(kRuntimeError, "corrupt AST");
        }

        if (!resolveColumns(child, node, tbl_ref)) {
          return false;
        }
      }

      return true;
    }

  }
}

}
