 /**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/parser/astutil.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/runtime/QueryBuilder.h>
#include <eventql/sql/runtime/runtime.h>
#include <eventql/sql/expressions/table/tablescan.h>

namespace csql {

TableScan::TableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    ScopedPtr<TableIterator> iter) :
    txn_(txn),
    iter_(std::move(iter)) {
  auto qbuilder = txn->getCompiler();

  for (const auto& slnode : stmt->selectList()) {
    QueryTreeUtil::resolveColumns(
        slnode->expression(),
        std::bind(
            &TableIterator::findColumn,
            iter_.get(),
            std::placeholders::_1));

    select_exprs_.emplace_back(
        qbuilder->buildValueExpression(txn, slnode->expression()));
  }

  if (!stmt->whereExpression().isEmpty()) {
    QueryTreeUtil::resolveColumns(
        stmt->whereExpression().get(),
        std::bind(
            &TableIterator::findColumn,
            iter_.get(),
            std::placeholders::_1));

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
