 /**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/sql/parser/astutil.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/runtime/QueryBuilder.h>
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/expressions/table/tablescan.h>

namespace csql {

TableScan::TableScan(
    Transaction* txn,
    ExecutionContext* execution_context,
    RefPtr<SequentialScanNode> stmt,
    ScopedPtr<TableIterator> iter) :
    txn_(txn),
    execution_context_(execution_context),
    iter_(std::move(iter)) {
  auto qbuilder = txn->getCompiler();

  for (const auto& slnode : stmt->selectList()) {
    select_exprs_.emplace_back(
        qbuilder->buildValueExpression(txn, slnode->expression()));
  }

  if (!stmt->whereExpression().isEmpty()) {
    where_expr_ = std::move(Option<ValueExpression>(
        qbuilder->buildValueExpression(txn, stmt->whereExpression().get())));
  }
}

ScopedPtr<ResultCursor> TableScan::execute() {
  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &TableScan::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t TableScan::getNumColumns() const {
  return iter_->numColumns();
}

bool TableScan::next(SValue* out, int out_len) {
  Vector<SValue> buf(iter_->numColumns());

  while (iter_->nextRow(buf.data())) {
    if (!where_expr_.isEmpty()) {
      SValue pred;
      VM::evaluate(
          txn_,
          where_expr_.get().program(),
          buf.size(),
          buf.data(),
          &pred);

      if (!pred.getBool()) {
        continue;
      }
    }

    for (int i = 0; i < select_exprs_.size() && i < out_len; ++i) {
      VM::evaluate(
          txn_,
          select_exprs_[i].program(),
          buf.size(),
          buf.data(),
          &out[i]);
    }

    return true;
  }

  return false;
}

}
