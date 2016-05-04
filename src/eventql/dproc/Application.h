/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_APPLICATION_H
#define _FNORD_DPROC_APPLICATION_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/dproc/Task.h>
#include <eventql/dproc/TaskSpec.pb.h>
#include <eventql/util/protobuf/msg.h>

using namespace stx;

namespace dproc {

class Application : public RefCounted {
public:

  virtual String name() const = 0;

  virtual RefPtr<Task> getTaskInstance(const TaskSpec& spec) = 0;

  RefPtr<Task> getTaskInstance(const String& task_name, const Buffer& params);

};

class DefaultApplication : public Application {
public:

  String name() const override;

  DefaultApplication(const String& name);

  RefPtr<Task> getTaskInstance(const TaskSpec& spec) override;

  template <typename ProtoType>
  void registerProtoRDDFactory(
      const String& name,
      ProtoRDDFactory<ProtoType> factory);

  void registerTaskFactory(const String& name, TaskFactory factory);

  template <typename TaskType, typename... ArgTypes>
  void registerTask(const String& name, ArgTypes... args);

  template <typename TaskType, typename... ArgTypes>
  void registerProtoRDD(const String& name, ArgTypes... args);

protected:
  String name_;
  HashMap<String, TaskFactory> factories_;
};

template <typename ProtoType>
void DefaultApplication::registerProtoRDDFactory(
    const String& name,
    ProtoRDDFactory<ProtoType> factory) {
  registerTaskFactory(name, [factory] (const Buffer& params) -> RefPtr<Task> {
    return factory(msg::decode<ProtoType>(params));
  });
}

template <typename TaskType, typename... ArgTypes>
void DefaultApplication::registerTask(const String& name, ArgTypes... args) {
  registerTaskFactory(name, [=] (const Buffer& params) -> RefPtr<Task> {
    return new TaskType(params, args...);
  });
}

template <typename TaskType, typename... ArgTypes>
void DefaultApplication::registerProtoRDD(
    const String& name,
    ArgTypes... args) {
  registerTaskFactory(name, [=] (const Buffer& params) -> RefPtr<Task> {
    return new TaskType(msg::decode<typename TaskType::ParamType>(params), args...);
  });
}

} // namespace dproc

#endif
