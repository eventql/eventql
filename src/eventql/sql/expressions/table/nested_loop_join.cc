/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/nested_loop_join.h>

namespace csql {

NestedLoopJoin::NestedLoopJoin(
    Transaction* txn,
    JoinType join_type,
    const Vector<JoinNode::InputColumnRef>& input_map,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> join_cond_expr,
    Option<ValueExpression> where_expr,
    ScopedPtr<TableExpression> base_tbl,
    ScopedPtr<TableExpression> joined_tbl) :
    txn_(txn),
    join_type_(join_type),
    input_map_(input_map),
    select_exprs_(std::move(select_expressions)),
    join_cond_expr_(std::move(join_cond_expr)),
    where_expr_(std::move(where_expr)),
    base_tbl_(std::move(base_tbl)),
    joined_tbl_(std::move(joined_tbl)),
    joined_tbl_pos_(0) {}

static const size_t kMaxInMemoryRows = 1000000;

ScopedPtr<ResultCursor> NestedLoopJoin::execute() {
  auto joined_cursor = joined_tbl_->execute();
  Vector<SValue> row(joined_cursor->getNumColumns());
  while (joined_cursor->next(row.data(), row.size())) {
    joined_tbl_data_.emplace_back(row);
  }

  base_tbl_cursor_ = base_tbl_->execute();
  base_tbl_row_.resize(base_tbl_cursor_->getNumColumns());

  switch (join_type_) {
    case JoinType::OUTER:
      return executeOuterJoin();
    case JoinType::INNER:
      if (join_cond_expr_.isEmpty()) {
        /* fallthrough */
      } else {
        return executeInnerJoin();
      }
    case JoinType::CARTESIAN:
      return executeCartesianJoin();
    default:
      RAISE(kIllegalStateError);
  }
}


//bool NestedLoopJoin::onInputRow(
//    const TaskID& input_id,
//    const SValue* row,
//    int row_len) {
//  if (base_tbl_ids_.count(input_id) > 0) {
//    base_tbl_.emplace_back(row, row + row_len);
//    if (base_tbl_.size() >= kMaxInMemoryRows) {
//      RAISE(
//          kRuntimeError,
//          "Nested Loop JOIN intermediate result set is too large, try using an"
//          " equi-join instead.");
//    }
//    return true;
//  }
//
//  if (joined_tbl_ids_.count(input_id) > 0) {
//    joined_tbl_.emplace_back(row, row + row_len);
//    if (joined_tbl_.size() >= kMaxInMemoryRows) {
//      RAISE(
//          kRuntimeError,
//          "Nested Loop JOIN intermediate result set is too large, try using an"
//          " equi-join instead.");
//    }
//    return true;
//  }
//
//  RAISE(kIllegalStateError);
//}
//
//void NestedLoopJoin::onInputsReady() {
//
//  base_tbl_.clear();
//  joined_tbl_.clear();
//}

ScopedPtr<ResultCursor> NestedLoopJoin::executeCartesianJoin() {
  //Vector<SValue> outbuf(select_exprs_.size(), SValue{});
  //Vector<SValue> inbuf(input_map_.size(), SValue{});

  //for (const auto& r1 : base_tbl_) {
  //  for (const auto& r2 : joined_tbl_) {

  //    for (size_t i = 0; i < input_map_.size(); ++i) {
  //      const auto& m = input_map_[i];

  //      switch (m.table_idx) {
  //        case 0:
  //          inbuf[i] = r1[m.column_idx];
  //          break;
  //        case 1:
  //          inbuf[i] = r2[m.column_idx];
  //          break;
  //        default:
  //          RAISE(kRuntimeError, "invalid table index");
  //      }
  //    }

  //    if (!where_expr_.isEmpty()) {
  //      SValue pred;
  //      VM::evaluate(
  //          txn_,
  //          where_expr_.get().program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &pred);

  //      if (!pred.getBool()) {
  //        continue;
  //      }
  //    }

  //    for (int i = 0; i < select_exprs_.size(); ++i) {
  //      VM::evaluate(
  //          txn_,
  //          select_exprs_[i].program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &outbuf[i]);
  //    }

  //    //if (!input_(outbuf.data(), outbuf.size()))  {
  //    //  return;
  //    //}
  //  }
  //}
  RAISE(kNotYetImplementedError, "nyi");
}

ScopedPtr<ResultCursor> NestedLoopJoin::executeInnerJoin() {
  auto cursor = [this] (SValue* row, int row_len) -> bool {
    for (;;) {
      Vector<SValue> inbuf(input_map_.size(), SValue{});

      if (joined_tbl_pos_ == 0 || joined_tbl_pos_ == joined_tbl_data_.size()) {
        joined_tbl_pos_ = 0;

        if (!base_tbl_cursor_->next(
              base_tbl_row_.data(),
              base_tbl_row_.size())) {
          return false;
        }
      }

      while (joined_tbl_pos_ < joined_tbl_data_.size()) {
        const auto& joined_table_row = joined_tbl_data_[joined_tbl_pos_++];

        for (size_t i = 0; i < input_map_.size(); ++i) {
          const auto& m = input_map_[i];

          switch (m.table_idx) {
            case 0:
              inbuf[i] = base_tbl_row_[m.column_idx];
              break;
            case 1:
              inbuf[i] = joined_table_row[m.column_idx];
              break;
            default:
              RAISE(kRuntimeError, "invalid table index");
          }
        }

        {
          SValue pred;
          VM::evaluate(
              txn_,
              join_cond_expr_.get().program(),
              inbuf.size(),
              inbuf.data(),
              &pred);

          if (!pred.getBool()) {
            continue;
          }
        }

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

        for (int i = 0; i < select_exprs_.size() && i < row_len; ++i) {
          VM::evaluate(
              txn_,
              select_exprs_[i].program(),
              inbuf.size(),
              inbuf.data(),
              &row[i]);
        }

        return true;
      }
    }
  };

  return mkScoped(new DefaultResultCursor( select_exprs_.size(), cursor));
}

ScopedPtr<ResultCursor> NestedLoopJoin::executeOuterJoin() {
  //Vector<SValue> outbuf(select_exprs_.size(), SValue{});
  //Vector<SValue> inbuf(input_map_.size(), SValue{});

  //for (const auto& r1 : base_tbl_) {
  //  bool match = false;

  //  for (const auto& r2 : joined_tbl_) {
  //    for (size_t i = 0; i < input_map_.size(); ++i) {
  //      const auto& m = input_map_[i];

  //      switch (m.table_idx) {
  //        case 0:
  //          inbuf[i] = r1[m.column_idx];
  //          break;
  //        case 1:
  //          inbuf[i] = r2[m.column_idx];
  //          break;
  //        default:
  //          RAISE(kRuntimeError, "invalid table index");
  //      }
  //    }

  //    {
  //      SValue pred;
  //      VM::evaluate(
  //          txn_,
  //          join_cond_expr_.get().program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &pred);

  //      if (!pred.getBool()) {
  //        continue;
  //      }
  //    }

  //    if (!where_expr_.isEmpty()) {
  //      SValue pred;
  //      VM::evaluate(
  //          txn_,
  //          where_expr_.get().program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &pred);

  //      if (!pred.getBool()) {
  //        continue;
  //      }
  //    }

  //    match = true;

  //    for (int i = 0; i < select_exprs_.size(); ++i) {
  //      VM::evaluate(
  //          txn_,
  //          select_exprs_[i].program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &outbuf[i]);
  //    }

  //    //if (!input_(outbuf.data(), outbuf.size()))  {
  //    //  return;
  //    //}
  //  }

  //  if (!match) {
  //    for (size_t i = 0; i < input_map_.size(); ++i) {
  //      const auto& m = input_map_[i];

  //      if (m.table_idx != 0) {
  //        inbuf[i] = SValue();
  //      }
  //    }

  //    if (!where_expr_.isEmpty()) {
  //      SValue pred;
  //      VM::evaluate(
  //          txn_,
  //          where_expr_.get().program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &pred);

  //      if (!pred.getBool()) {
  //        continue;
  //      }
  //    }

  //    for (int i = 0; i < select_exprs_.size(); ++i) {
  //      VM::evaluate(
  //          txn_,
  //          select_exprs_[i].program(),
  //          inbuf.size(),
  //          inbuf.data(),
  //          &outbuf[i]);
  //    }

  //    //if (!input_(outbuf.data(), outbuf.size()))  {
  //    //  return;
  //    //}
  //  }
  //}
  RAISE(kNotYetImplementedError, "nyi");
}

//NestedLoopJoinFactory::NestedLoopJoinFactory(
//    JoinType join_type,
//    const Set<TaskID>& base_tbl_ids,
//    const Set<TaskID>& joined_tbl_ids,
//    const Vector<JoinNode::InputColumnRef>& input_map,
//    Vector<RefPtr<SelectListNode>> select_exprs,
//    Option<RefPtr<ValueExpressionNode>> join_cond_expr,
//    Option<RefPtr<ValueExpressionNode>> where_expr) :
//    join_type_(join_type),
//    base_tbl_ids_(base_tbl_ids),
//    joined_tbl_ids_(joined_tbl_ids),
//    input_map_(input_map),
//    select_exprs_(select_exprs),
//    join_cond_expr_(join_cond_expr),
//    where_expr_(where_expr) {}
//
//RefPtr<Task> NestedLoopJoinFactory::build(
//    Transaction* txn,
//    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
//  auto qbuilder = txn->getRuntime()->queryBuilder();
//
//  Vector<ValueExpression> select_expressions;
//  for (const auto& slnode : select_exprs_) {
//    select_expressions.emplace_back(
//        qbuilder->buildValueExpression(txn, slnode->expression()));
//  }
//
//  Option<ValueExpression> join_cond_expr;
//  if (!join_cond_expr_.isEmpty()) {
//    join_cond_expr = std::move(Option<ValueExpression>(
//        qbuilder->buildValueExpression(txn, join_cond_expr_.get())));
//  }
//
//  Option<ValueExpression> where_expr;
//  if (!where_expr_.isEmpty()) {
//    where_expr = std::move(Option<ValueExpression>(
//        qbuilder->buildValueExpression(txn, where_expr_.get())));
//  }
//
//  return new NestedLoopJoin(
//      txn,
//      join_type_,
//      base_tbl_ids_,
//      joined_tbl_ids_,
//      input_map_,
//      std::move(select_expressions),
//      std::move(join_cond_expr),
//      std::move(where_expr),
//      std::move(input));
//}

}
