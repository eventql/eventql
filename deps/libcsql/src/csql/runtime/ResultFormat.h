/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/runtime/queryplan.h>

namespace csql {

class ResultFormat : public RefCounted {
public:

  virtual void formatResults(
      RefPtr<QueryPlan> query,
      ExecutionContext* context) = 0;

};

class CallbackResultHandler : public ResultFormat {
public:

  void onStatementBegin(Function<void (size_t stmt_id)> fn);

  void onRow(
      Function<void (size_t stmt_id, int argc, const SValue* argv)> fn);

  void onStatementEnd(Function<void (size_t stmt_id)> fn);

  void formatResults(
      RefPtr<QueryPlan> query,
      ExecutionContext* context) override;

protected:
  Function<void (size_t stmt_id, int argc, const SValue* argv)> on_row_;
};

}
