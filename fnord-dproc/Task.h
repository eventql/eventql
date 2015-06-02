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
#include <fnord-base/buffer.h>
#include <fnord-base/option.h>
#include <fnord-base/exception.h>
#include <fnord-base/VFSFile.h>
#include <fnord-base/thread/future.h>
#include <fnord-msg/msg.h>

namespace fnord {
namespace dproc {

class Task;

typedef Function<RefPtr<Task> (const Buffer& params)> TaskFactory;

template <typename T>
using ProtoTaskFactory = Function<RefPtr<Task> (const T& params)>;

struct TaskDependency {
  String task_name;
  Buffer params;
};

class TaskContext {
public:

  virtual ~TaskContext() {}

  virtual RefPtr<VFSFile> getDependency(size_t index) = 0;
  virtual size_t numDependencies() const = 0;

};

class Task : public RefCounted {
public:

  virtual ~Task() {}

  virtual List<TaskDependency> dependencies() {
    return List<TaskDependency>{};
  }

  virtual RefPtr<VFSFile> run(TaskContext* context) = 0;

  virtual Vector<String> preferredLocations() {
    return Vector<String>{};
  }

  virtual Option<String> cacheKey() {
    return None<String>();
  }

};

template <typename _ParamType, typename _ResultType>
class ProtoTask : public Task {
public:
  typedef _ParamType ParamType;
  typedef _ResultType ResultType;

  virtual void run(TaskContext* context, ResultType* result) = 0;
  RefPtr<VFSFile> run(TaskContext* context) override;
};

template <typename ParamType, typename ResultType>
RefPtr<VFSFile> ProtoTask<ParamType, ResultType>::run(TaskContext* context) {
  ResultType result;
  run(context, &result);
  return msg::encode(result).get();
}

//template <typename _ParamType, typename _ResultType>
//ProtoTask<RefPtr<VFSFile> run() override;


//template <typename ProtoType>
//template <typename... ArgTypes>
//ProtoTask<ProtoType>::ProtoTask(
//    const Buffer& buf,
//    ArgTypes... args) :
//    ProtoTask<ProtoType>(ProtoType{}, args...) {}

} // namespace dproc
} // namespace fnord

#endif
