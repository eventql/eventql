/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/NestedLoopJoin.h>

namespace csql {

NestedLoopJoin::NestedLoopJoin(
    Transaction* txn,
    JoinType join_type,
    ScopedPtr<Task> base_tbl,
    ScopedPtr<Task> joined_tbl,
    const Vector<JoinNode::InputColumnRef>& input_map,
    const Vector<String>& column_names,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> join_cond_expr,
    Option<ValueExpression> where_expr) :
    txn_(txn),
    join_type_(join_type),
    base_table_(std::move(base_tbl)),
    joined_table_(std::move(joined_tbl)),
    input_map_(input_map),
    column_names_(column_names),
    select_exprs_(std::move(select_expressions)),
    join_cond_expr_(std::move(join_cond_expr)),
    where_expr_(std::move(where_expr)) {}

static const size_t kMaxInMemoryRows = 1000000;

//void NestedLoopJoin::execute(
//    ExecutionContext* context,
//    Function<bool (int argc, const SValue* argv)> fn) {
//  List<Vector<SValue>> base_tbl_data;
//  base_table_->execute(
//      context,
//      [&base_tbl_data] (int argc, const SValue* argv) -> bool {
//    base_tbl_data.emplace_back(argv, argv + argc);
//    if (base_tbl_data.size() >= kMaxInMemoryRows) {
//      RAISE(
//          kRuntimeError,
//          "Nested Loop JOIN intermediate result set is too large, try using an"
//          " equi-join instead.");
//    }
//    return true;
//  });
//  context->incrNumSubtasksCompleted(1);
//
//  List<Vector<SValue>> joined_tbl_data;
//  joined_table_->execute(
//      context,
//      [&joined_tbl_data] (int argc, const SValue* argv) -> bool {
//    joined_tbl_data.emplace_back(argv, argv + argc);
//    if (joined_tbl_data.size() >= kMaxInMemoryRows) {
//      RAISE(
//          kRuntimeError,
//          "Nested Loop JOIN intermediate result set is too large, try using an"
//          " equi-join instead.");
//    }
//    return true;
//  });
//  context->incrNumSubtasksCompleted(1);
//
//  switch (join_type_) {
//    case JoinType::OUTER:
//      executeOuterJoin(
//          context,
//          fn,
//          base_tbl_data,
//          joined_tbl_data);
//      break;
//    case JoinType::INNER:
//      if (join_cond_expr_.isEmpty()) {
//        /* fallthrough */
//      } else {
//        executeInnerJoin(
//            context,
//            fn,
//            base_tbl_data,
//            joined_tbl_data);
//        break;
//      }
//    case JoinType::CARTESIAN:
//      executeCartesianJoin(
//          context,
//          fn,
//          base_tbl_data,
//          joined_tbl_data);
//      break;
//  }
//
//  context->incrNumSubtasksCompleted(1);
//}

void NestedLoopJoin::executeCartesianJoin(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn,
    const List<Vector<SValue>>& t1,
    const List<Vector<SValue>>& t2) {
  Vector<SValue> outbuf(select_exprs_.size(), SValue{});
  Vector<SValue> inbuf(input_map_.size(), SValue{});

  for (const auto& r1 : t1) {
    for (const auto& r2 : t2) {

      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            inbuf[i] = r1[m.column_idx];
            break;
          case 1:
            inbuf[i] = r2[m.column_idx];
            break;
          default:
            RAISE(kRuntimeError, "invalid table index");
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

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            inbuf.size(),
            inbuf.data(),
            &outbuf[i]);
      }

      if (!fn(outbuf.size(), outbuf.data()))  {
        return;
      }
    }
  }
}

void NestedLoopJoin::executeInnerJoin(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn,
    const List<Vector<SValue>>& t1,
    const List<Vector<SValue>>& t2) {
  Vector<SValue> outbuf(select_exprs_.size(), SValue{});
  Vector<SValue> inbuf(input_map_.size(), SValue{});

  for (const auto& r1 : t1) {
    for (const auto& r2 : t2) {

      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            inbuf[i] = r1[m.column_idx];
            break;
          case 1:
            inbuf[i] = r2[m.column_idx];
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

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            inbuf.size(),
            inbuf.data(),
            &outbuf[i]);
      }

      if (!fn(outbuf.size(), outbuf.data()))  {
        return;
      }
    }
  }
}

void NestedLoopJoin::executeOuterJoin(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn,
    const List<Vector<SValue>>& t1,
    const List<Vector<SValue>>& t2) {
  Vector<SValue> outbuf(select_exprs_.size(), SValue{});
  Vector<SValue> inbuf(input_map_.size(), SValue{});

  for (const auto& r1 : t1) {
    bool match = false;

    for (const auto& r2 : t2) {
      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            inbuf[i] = r1[m.column_idx];
            break;
          case 1:
            inbuf[i] = r2[m.column_idx];
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

      match = true;

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            inbuf.size(),
            inbuf.data(),
            &outbuf[i]);
      }

      if (!fn(outbuf.size(), outbuf.data()))  {
        return;
      }
    }

    if (!match) {
      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        if (m.table_idx != 0) {
          inbuf[i] = SValue();
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

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            inbuf.size(),
            inbuf.data(),
            &outbuf[i]);
      }

      if (!fn(outbuf.size(), outbuf.data()))  {
        return;
      }
    }
  }
}

}
