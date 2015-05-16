/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/io/file.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/logging.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-dproc/LocalScheduler.h>

namespace fnord {
namespace dproc {

LocalScheduler::LocalScheduler(
    const String& tempdir /* = "/tmp" */,
    size_t max_threads /* = 8 */) :
    tempdir_(tempdir),
    tpool_(max_threads) {}

void LocalScheduler::start() {
  tpool_.start();
}

void LocalScheduler::stop() {
  tpool_.stop();
}

RefPtr<VFSFile> LocalScheduler::run(
    Application* app,
    TaskSpec task) {
  const auto& params = task.params();
  return run(app, task.task_name(), Buffer(params.data(), params.size()));
}

RefPtr<VFSFile> LocalScheduler::run(
    Application* app,
    const String& task,
    const Buffer& params) {
  RefPtr<LocalTaskRef> head_task(
      new LocalTaskRef(app->getTaskInstance(task, params)));

  LocalTaskPipeline pipeline;
  pipeline.tasks.push_back(head_task);
  run(app, &pipeline);

  BufferRef buf(new Buffer());
  return buf.get();
}

void LocalScheduler::run(
    Application* app,
    LocalTaskPipeline* pipeline) {
  fnord::logInfo(
      "fnord.dproc",
      "Starting local pipeline id=$0 tasks=$1",
      (void*) pipeline,
      pipeline->tasks.size());

  std::unique_lock<std::mutex> lk(pipeline->mutex);

  while (pipeline->tasks.size() > 0) {
    bool waiting = true;
    size_t num_waiting = 0;
    size_t num_running = 0;
    size_t num_completed = 0;

    for (auto& taskref : pipeline->tasks) {
      if (taskref->finished) {
        ++num_completed;
        continue;
      }

      if (taskref->running) {
        ++num_running;
        continue;
      }

      if (!taskref->expanded) {
        taskref->expanded = true;

        auto parent_task = taskref;
        for (const auto& dep : taskref->task->dependencies()) {
          RefPtr<LocalTaskRef> depref(new LocalTaskRef(
              app->getTaskInstance(dep.task_name, dep.params)));
          parent_task->dependencies.emplace_back(depref);
          pipeline->tasks.emplace_back(depref);
        }

        waiting = false;
        break;
      }

      bool deps_finished = true;
      for (const auto& dep : taskref->dependencies) {
        if (!dep->finished) {
          deps_finished = false;
        }
      }

      if (!deps_finished) {
        ++num_waiting;
        continue;
      }

      taskref->running = true;
      tpool_.run(std::bind(&LocalScheduler::runTask, this, pipeline, taskref));
      waiting = false;
    }

    fnord::logInfo(
        "fnord.dproc",
        "Running local pipeline... id=$0 tasks=$1, running=$2, waiting=$3, completed=$4",
        (void*) pipeline,
        pipeline->tasks.size(),
        num_running,
        num_waiting,
        num_completed);

    if (waiting) {
      pipeline->wakeup.wait(lk);
    }

    while (pipeline->tasks.size() > 0 && pipeline->tasks.back()->finished) {
      pipeline->tasks.pop_back();
    }
  }

  fnord::logInfo(
      "fnord.dproc",
      "Completed local pipeline id=$0",
      (void*) pipeline);
}

void LocalScheduler::runTask(
    LocalTaskPipeline* pipeline,
    RefPtr<LocalTaskRef> task) {
  auto output_file = FileUtil::joinPaths(tempdir_, rnd_.hex128() + ".tmp");

  try {
    auto res = task->task->run(task.get());

    auto file = File::openFile(
        output_file + "~",
        File::O_CREATEOROPEN | File::O_WRITE);

    file.write(res->data(), res->size());
    FileUtil::mv(output_file + "~", output_file);
  } catch (const std::exception& e) {
    fnord::logError("fnord.dproc", e, "error");
  }

  std::unique_lock<std::mutex> lk(pipeline->mutex);
  task->output_filename = output_file;
  task->finished = true;
  lk.unlock();
  pipeline->wakeup.notify_all();
}

LocalScheduler::LocalTaskRef::LocalTaskRef(RefPtr<Task> _task) :
    task(_task),
    running(false),
    expanded(false),
    finished(false) {}

RefPtr<VFSFile> LocalScheduler::LocalTaskRef::getDependency(size_t index) {
  if (index >= dependencies.size()) {
    RAISEF(kIndexError, "invalid dependecy index: $0", index);
  }

  const auto& dep = dependencies[index];
  if (!FileUtil::exists(dep->output_filename)) {
    RAISEF(kRuntimeError, "missing upstream output: $0", dep->output_filename);
  }

  return RefPtr<VFSFile>(
      new io::MmappedFile(
          File::openFile(dep->output_filename, File::O_READ)));
}

size_t LocalScheduler::LocalTaskRef::numDependencies() const {
  return dependencies.size();
}

} // namespace dproc
} // namespace fnord
