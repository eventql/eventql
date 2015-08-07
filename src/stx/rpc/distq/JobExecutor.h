/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/autoref.h"
#include "stx/duration.h"
#include "stx/exception.h"
#include "stx/Serializable.h"
#include "stx/io/inputstream.h"

namespace stx {
namespace distq {

class JobExecutor {
public:

  virtual ~JobExecutor() {}

  virtual RefPtr<Job> getJob(
      const String& method,
      const Serializable& params) = 0;

};


class LocalExecutor : public JobExecutor {
public:

  RefPtr<Job> getJob(
      const String& method,
      const Serializable& params) override;

};

} // namespace distq
} // namespace stx

