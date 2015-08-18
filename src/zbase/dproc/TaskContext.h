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
#include <stx/autoref.h>
#include <zbase/dproc/Task.h>

using namespace stx;

class TaskContext {
public:

  virtual ~TaskContext() {}

  virtual RefPtr<TaskRef> getDependency(size_t index) = 0;

  virtual size_t numDependencies() const = 0;

  virtual bool isCancelled() const = 0;

};

