/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_LOCALSCHEDULER_H
#define _FNORD_DPROC_LOCALSCHEDULER_H
#include "stx/stdtypes.h"
#include "stx/random.h"
#include "stx/thread/taskscheduler.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include <zbase/dproc/Application.h>
#include <zbase/dproc/Scheduler.h>
#include <zbase/dproc/TaskSpec.pb.h>
#include <zbase/dproc/TaskRef.h>

using namespace stx;

namespace dproc {

class LocalScheduler : public Scheduler {
public:

  LocalScheduler(
      const String& tempdir = "/tmp",
      size_t max_requests = 32);

  RefPtr<TaskResultFuture> run(
      RefPtr<Application> app,
      const TaskSpec& task) override;

  void start();
  void stop();

protected:

  class LocalTaskContext : public TaskContext, public RefCounted {
  public:
    LocalTaskContext(
        RefPtr<Application> app,
        const String& task_name,
        const Buffer& params);

    RefPtr<dproc::TaskRef> getDependency(size_t index) override;
    size_t numDependencies() const override;
    bool isCancelled() const override;

    void readCache();
    void cancel();

    Function<RefPtr<Task> ()> task_factory;
    RefPtr<TaskRef> task_ref;
    String cache_filename;
    String debug_name;
    std::atomic<bool> running;
    std::atomic<bool> finished;
    std::atomic<bool> failed;
    std::atomic<bool> expanded;
    std::atomic<bool> cancelled;

    Vector<RefPtr<LocalTaskContext>> dependencies;
  };

  struct LocalTaskPipeline : public RefCounted {
    LocalTaskPipeline() : cur_tasks(0), max_tasks(1) {}
    Vector<RefPtr<LocalTaskContext>> tasks;
    std::mutex mutex;
    std::condition_variable wakeup;
    size_t cur_tasks;
    size_t max_tasks;
  };

  void runPipeline(
      Application* app,
      RefPtr<LocalTaskPipeline> pipeline,
      RefPtr<TaskResultFuture> result);

  void runTask(
      RefPtr<LocalTaskPipeline> pipeline,
      RefPtr<LocalTaskContext> task,
      RefPtr<TaskResultFuture> result);

  String tempdir_;
  size_t max_threads_;
  size_t num_threads_;
  Random rnd_;
  thread::CachedThreadPool work_tpool_;
  thread::FixedSizeThreadPool req_tpool_;
};

} // namespace dproc

#endif
