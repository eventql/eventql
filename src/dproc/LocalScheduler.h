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
#include "fnord/stdtypes.h"
#include "fnord/random.h"
#include "fnord/thread/taskscheduler.h"
#include "fnord/thread/FixedSizeThreadPool.h"
#include <dproc/Application.h>
#include <dproc/Scheduler.h>
#include <dproc/TaskSpec.pb.h>

using namespace fnord;

namespace dproc {

class LocalScheduler : public Scheduler {
public:

  LocalScheduler(
      const String& tempdir = "/tmp",
      size_t max_threads = 8,
      size_t max_requests = 32);

  RefPtr<TaskResultFuture> run(
      RefPtr<Application> app,
      const TaskSpec& task) override;

  void start();
  void stop();

protected:

  class LocalTaskRef : public TaskContext, public RefCounted {
  public:
    LocalTaskRef(
        RefPtr<Application> app,
        const String& task_name,
        const Buffer& params);

    RefPtr<dproc::RDD> getDependency(size_t index) override;
    size_t numDependencies() const override;
    bool isCancelled() const override;

    void readCache();
    void cancel();

    RefPtr<Task> task;
    String cache_filename;
    String debug_name;
    std::atomic<bool> running;
    std::atomic<bool> finished;
    std::atomic<bool> failed;
    std::atomic<bool> expanded;
    std::atomic<bool> cancelled;

    Vector<RefPtr<LocalTaskRef>> dependencies;
  };

  struct LocalTaskPipeline {
    Vector<RefPtr<LocalTaskRef>> tasks;
    std::mutex mutex;
    std::condition_variable wakeup;
  };

  void runPipeline(
      Application* app,
      LocalTaskPipeline* pipeline,
      RefPtr<TaskResultFuture> result);

  void runTask(
      LocalTaskPipeline* pipeline,
      RefPtr<LocalTaskRef> task,
      RefPtr<TaskResultFuture> result);

  String tempdir_;
  size_t max_threads_;
  size_t num_threads_;
  Random rnd_;
  thread::FixedSizeThreadPool tpool_;
  thread::FixedSizeThreadPool req_tpool_;
};

} // namespace dproc

#endif
