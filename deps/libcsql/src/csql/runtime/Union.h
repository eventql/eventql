/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class Union : public Task {
public:

  Union(Vector<ScopedPtr<Task>> sources);

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  Vector<ScopedPtr<Task>> sources_;
};

}
