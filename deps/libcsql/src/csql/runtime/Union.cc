/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/Union.h>
#include <csql/runtime/EmptyTable.h>

namespace csql {

Union::Union(
    Vector<ScopedPtr<TableExpression>> sources) :
    sources_(std::move(sources)) {
  if (sources_.size() == 0) {
    RAISE(kRuntimeError, "UNION must have at least one source table");
  }

  size_t ncols = size_t(-1);

  for (const auto& cur : sources_) {
    if (dynamic_cast<EmptyTable*>(cur.get())) {
      continue;
    }

    auto tcols = cur->numColumns();
    if (ncols != size_t(-1) && ncols != tcols) {
      RAISE(kRuntimeError, "UNION tables return different number of columns");
    }

    ncols = tcols;
  }
}

void Union::prepare(ExecutionContext* context) {
  for (auto& source : sources_) {
    source->prepare(context);
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
  return sources_[0]->columnNames();
}

size_t Union::numColumns() const {
  return sources_[0]->numColumns();
}

}
