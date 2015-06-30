/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/Union.h>

namespace csql {


Union::Union(
    Vector<ScopedPtr<TableExpression>> sources) :
    sources_(std::move(sources)) {
  if (sources_.size() == 0) {
    RAISE(kRuntimeError, "UNION must have at least one source table");
  }

  for (const auto& s : sources_) {
    if (s->numColunns() != sources_[0]->numColunns()) {
      RAISE(kRuntimeError, "UNION tables return different number of columns");
    }
  }
}

void Union::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {

  for (auto& source : sources_) {
    source->execute(
        context,
        [fn] (int sargc, const SValue* sargv) -> bool {
      return fn(sargc, sargv);
    });
  }
}

Vector<String> Union::columnNames() const {
  sources_[0]->columnNames();
}

size_t Union::numColunns() const {
  sources_[0]->numColunns();
}

}
