/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/parser/token.h>
#include <csql/runtime/compiler.h>
#include <csql/runtime/vm.h>
#include <csql/runtime/importstatement.h>
#include <csql/svalue.h>
#include <stx/exception.h>

namespace csql {

ImportStatement::ImportStatement(ASTNode* ast, ValueExpressionBuilder* compiler) {
  RAISE(kNotImplementedError);
  //if (ast->getChildren().size() < 2) {
  //  RAISE(kRuntimeError, "corrupt ast: ASTNode::Import\n");
  //}

  //auto source_uri_sval = executeSimpleConstExpression(
  //    compiler,
  //    ast->getChildren().back());

  //source_uri_ = source_uri_sval.toString();

  //const auto& children = ast->getChildren();
  //for (int i = 0; i < children.size() - 1; ++i) {
  //  tables_.emplace_back(children[i]->getToken()->getString());
  //}
}

const std::string& ImportStatement::source_uri() const {
  return source_uri_;
}

const std::vector<std::string>& ImportStatement::tables() const {
  return tables_;
}

}
