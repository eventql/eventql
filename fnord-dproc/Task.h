/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_TASK_H
#define _FNORD_DPROC_TASK_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>

namespace fnord {
namespace dproc {

class Task;

typedef Function<RefPtr<Task> (const Buffer& params)> TaskFactory;

class TaskDependency : public RefCounted {
public:
  TaskDependency(RefPtr<Task> task);
  RefPtr<Task> task() const;
protected:
  RefPtr<Task> task_;
};

class Task : public RefCounted {
public:

  virtual List<TaskDependency> dependencies();

  virtual void run(const Buffer* output) = 0;

  virtual Option<String> cacheKey();

  virtual Vector<String> preferredLocations();

};

} // namespace cstable
} // namespace fnord

#endif
