/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_SCHEDULER_H
#define _FNORD_SCHEDULER_H
#include "fnord/stdtypes.h"
#include <dproc/Application.h>
#include <dproc/TaskSpec.pb.h>
#include <dproc/TaskResultFuture.h>

namespace fnord {
namespace dproc {

class Scheduler : public RefCounted {
public:

  virtual ~Scheduler() {}

  virtual RefPtr<TaskResultFuture> run(
      RefPtr<Application> app,
      const TaskSpec& task) = 0;

};

} // namespace dproc
} // namespace fnord

#endif
