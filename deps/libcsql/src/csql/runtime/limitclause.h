/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/tasks/Task.h>

namespace csql {

class LimitClause : public Task {
public:

  LimitClause(int limit, int offset, ScopedPtr<Task> child);

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  size_t limit_;
  size_t offset_;
  ScopedPtr<Task> child_;
};

}
