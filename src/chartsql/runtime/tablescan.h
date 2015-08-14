/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_QUERY_TABLELESCAN_H
#define _FNORDMETRIC_QUERY_TABLELESCAN_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <chartsql/parser/token.h>
#include <chartsql/parser/astnode.h>
#include <chartsql/runtime/queryplannode.h>
#include <chartsql/runtime/tablerepository.h>
#include <chartsql/runtime/compiler.h>
#include <chartsql/runtime/vm.h>
#include <stx/exception.h>

namespace csql {

class TableScan : public QueryPlanNode {
public:

  TableScan(
      TableRef* tbl_ref,
      std::vector<std::string>&& columns,
      VM::Instruction* select_expr,
      VM::Instruction* where_expr);

  void execute() override;
  bool nextRow(SValue* row, int row_len) override;
  size_t getNumCols() const override;
  const std::vector<std::string>& getColumns() const override;

protected:

  static bool resolveColumns(ASTNode* node, ASTNode* parent, TableRef* tbl_ref);

  TableRef* const tbl_ref_;
  const std::vector<std::string> columns_;
  VM::Instruction* const select_expr_;
  VM::Instruction* const where_expr_;
};

}
#endif
