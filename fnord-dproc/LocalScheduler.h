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
#include "fnord-base/stdtypes.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-base/thread/FixedSizeThreadPool.h"
#include <fnord-dproc/Application.h>
#include <fnord-dproc/TaskSpec.pb.h>

namespace fnord {
namespace dproc {

class LocalScheduler {
public:

  LocalScheduler(
      const String& tempdir = "/tmp",
      size_t max_threads = 8);

  RefPtr<VFSFile> run(
      Application* app,
      TaskSpec task);

  RefPtr<VFSFile> run(
      Application* app,
      const String& task,
      const Buffer& params);

  void start();
  void stop();

protected:

  class LocalTaskRef : public TaskContext, public RefCounted {
  public:
    LocalTaskRef(RefPtr<Task> _task);
    RefPtr<VFSFile> getDependency(size_t index) override;
    size_t numDependencies() const override;

    RefPtr<Task> task;
    String output_filename;
    bool running;
    bool finished;
    bool expanded;
    Vector<RefPtr<LocalTaskRef>> dependencies;
  };

  struct LocalTaskPipeline {
    Vector<RefPtr<LocalTaskRef>> tasks;
    std::mutex mutex;
    std::condition_variable wakeup;
  };

  void run(Application* app, LocalTaskPipeline* pipeline);
  void runTask(LocalTaskPipeline* pipeline, RefPtr<LocalTaskRef> task);

  String tempdir_;
  size_t max_threads_;
  size_t num_threads_;
  Random rnd_;
  thread::FixedSizeThreadPool tpool_;
};

} // namespace dproc
} // namespace fnord

#endif
