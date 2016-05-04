 /**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/parser/astutil.h>
#include <csql/qtree/QueryTreeUtil.h>
#include <csql/runtime/QueryBuilder.h>
#include <csql/runtime/runtime.h>
#include <csql/runtime/tablescan.h>

namespace csql {

TableScan::TableScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> stmt,
    ScopedPtr<TableIterator> iter,
    RowSinkFn output) :
    txn_(txn),
    iter_(std::move(iter)),
    output_(output) {
  auto qbuilder = txn->getRuntime()->queryBuilder();

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

void TableScan::onInputsReady() {
  Vector<SValue> inbuf(iter_->numColumns());
  Vector<SValue> outbuf(select_exprs_.size());
  while (iter_->nextRow(inbuf.data())) {
    if (!where_expr_.isEmpty()) {
      SValue pred;
      VM::evaluate(
          txn_,
          where_expr_.get().program(),
          inbuf.size(),
          inbuf.data(),
          &pred);

      if (!pred.getBool()) {
        continue;
      }
    }

    for (int i = 0; i < select_exprs_.size(); ++i) {
      VM::evaluate(
          txn_,
          select_exprs_[i].program(),
          inbuf.size(),
          inbuf.data(),
          &outbuf[i]);
    }

    if (!output_(outbuf.data(), outbuf.size())) {
      return;
    }
  }
}

}
