/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/dproc/DispatchService.h>

using namespace stx;

namespace dproc {

void DispatchService::registerApp(
    RefPtr<Application> app,
    RefPtr<Scheduler> scheduler) {
  apps_.emplace(
      app->name(),
      AppRef {
        .app = app,
        .scheduler = scheduler
      });
}

RefPtr<TaskResultFuture> DispatchService::run(const TaskSpec& task) {
  auto iter = apps_.find(task.application());
  if (iter == apps_.end()) {
    RAISEF(kRuntimeError, "application not found: '$0'", task.application());
  }

  auto& app_ref = iter->second;
  return app_ref.scheduler->run(app_ref.app, task);
}

} // namespace dproc

