/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/dproc/Application.h>

using namespace stx;

namespace dproc {

RefPtr<Task> Application::getTaskInstance(
    const String& task_name,
    const Buffer& params) {
  TaskSpec spec;
  spec.set_task_name(task_name);
  spec.set_params((char*) params.data(), params.size());
  return getTaskInstance(spec);
}

DefaultApplication::DefaultApplication(const String& name) : name_(name) {}

String DefaultApplication::name() const {
  return name_;
}

RefPtr<Task> DefaultApplication::getTaskInstance(const TaskSpec& spec) {
  auto factory = factories_.find(spec.task_name());

  if (factory == factories_.end()) {
    RAISEF(kIndexError, "unknown task: '$0'", spec.task_name());
  }

  return factory->second(Buffer(spec.params().data(), spec.params().size()));
}

void DefaultApplication::registerTaskFactory(
    const String& name,
    TaskFactory factory) {
  factories_.emplace(name, factory);
}

} // namespace dproc

