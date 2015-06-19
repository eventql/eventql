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
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <fnord/buffer.h>
#include <fnord/option.h>
#include <fnord/exception.h>
#include <fnord/SHA1.h>
#include <fnord/VFSFile.h>
#include <fnord/thread/future.h>
#include <fnord/protobuf/msg.h>

using namespace fnord;

namespace dproc {

class Task;
class TaskContext;

typedef Function<RefPtr<Task> (const Buffer& params)> TaskFactory;

template <typename T>
using ProtoRDDFactory = Function<RefPtr<Task> (const T& params)>;

struct TaskDependency {
  String task_name;
  Buffer params;
};

class Task : public RefCounted {
public:

  Task() {}
  virtual ~Task() {}

  virtual void compute(TaskContext* context) = 0;

  virtual List<TaskDependency> dependencies() const {
    return List<TaskDependency>{};
  }

  virtual Vector<String> preferredLocations() const {
    return Vector<String>{};
  }

  virtual String contentType() const {
    return "application/octet-stream";
  }

  virtual RefPtr<VFSFile> encode() const = 0;
  virtual void decode(RefPtr<VFSFile> data) = 0;

  virtual Option<String> cacheKey() const {
    return None<String>();
  }

  virtual Option<String> cacheKeySHA1() const {
    auto key = cacheKey();

    if (key.isEmpty()) {
      return key;
    } else {
      return Some(SHA1::compute(key.get()).toString());
    }
  }

};

using RDD = Task;

class TaskContext {
public:

  virtual ~TaskContext() {}

  template <typename TaskType>
  RefPtr<TaskType> getDependencyAs(size_t index);

  virtual RefPtr<RDD> getDependency(size_t index) = 0;

  virtual size_t numDependencies() const = 0;

  virtual bool isCancelled() const = 0;

};

template <typename _ProtoType>
class ProtoRDD : public RDD {
public:
  typedef _ProtoType ProtoType;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  ProtoType* data();

protected:
  ProtoType data_;
};

template <typename TaskType>
RefPtr<TaskType> TaskContext::getDependencyAs(size_t index) {
  return getDependency(index).asInstanceOf<TaskType>();
}

template <typename ProtoType>
RefPtr<VFSFile> ProtoRDD<ProtoType>::encode() const {
  return msg::encode(data_).get();
}

template <typename ProtoType>
void ProtoRDD<ProtoType>::decode(RefPtr<VFSFile> data) {
  msg::decode(data->data(), data->size(), &data_);
}

} // namespace dproc

#endif
