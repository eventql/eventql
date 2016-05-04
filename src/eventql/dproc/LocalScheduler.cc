/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/dproc/LocalScheduler.h>
#include <eventql/dproc/TaskRef.h>

using namespace stx;

namespace dproc {

LocalScheduler::LocalScheduler(
    const String& tempdir /* = "/tmp" */,
    size_t max_requests /* = 32 */) :
    tempdir_(tempdir),
    work_tpool_(thread::ThreadPoolOptions{}),
    req_tpool_(thread::ThreadPoolOptions{}, max_requests) {}

void LocalScheduler::start() {
  req_tpool_.start();
}

void LocalScheduler::stop() {
  req_tpool_.stop();
}

RefPtr<TaskResultFuture> LocalScheduler::run(
    RefPtr<Application> app,
    const TaskSpec& task) {
  auto task_id = rnd_.hex64();

  stx::logDebug(
      "dproc",
      "Starting job; id=$0",
      task_id);

  RefPtr<TaskResultFuture> result(new TaskResultFuture());

  try {
    auto instance = mkRef(
        new LocalTaskContext(
            app,
            task.task_name(),
            Buffer(task.params().data(), task.params().size())));

    req_tpool_.run([this, app, result, instance, task_id] () {
      try {
        auto pipeline = mkRef(new LocalTaskPipeline());
        pipeline->tasks.push_back(instance);
        pipeline->max_tasks = 6;

        result->onCancel([pipeline] {
          std::unique_lock<std::mutex> lk(pipeline->mutex);

          for (auto& taskref : pipeline->tasks) {
            taskref->cancel();
          }
        });

        runPipeline(app.get(), pipeline, result);

        if (instance->failed) {
          RAISE(kRuntimeError, "task failed");
        }

        stx::logDebug(
            "dproc",
            "Job successfully completed; id=$0",
            task_id);

        result->returnResult(instance->task_ref);
      } catch (const StandardException& e) {
        stx::logError(
            "dproc",
            e,
            "Job failed; id=$0",
            task_id);

        result->returnError(e);
      }
    });
  } catch (const StandardException& e) {
    stx::logError(
        "dproc",
        e,
        "Job failed; id=$0",
        task_id);

    result->returnError(e);
  }

  return result;
}

void LocalScheduler::runPipeline(
    Application* app,
    RefPtr<LocalTaskPipeline> pipeline,
    RefPtr<TaskResultFuture> result) {
  std::unique_lock<std::mutex> lk(pipeline->mutex);
  result->updateStatus([pipeline] (TaskStatus* status) {
    status->num_subtasks_total = pipeline->tasks.size();
  });

  while (pipeline->tasks.size() > 0) {
    if (result->isCancelled()) {
      for (auto& taskref : pipeline->tasks) {
        taskref->cancel();
      }

      RAISE(kFutureError, "future cancelled");
    }

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

        auto task = taskref->task_ref->getInstance();
        auto cache_key = task->cacheKeySHA1();
        if (cache_key.isEmpty()) {
          taskref->cache_filename = FileUtil::joinPaths(
              tempdir_,
              StringUtil::format("$0.tmp", rnd_.sha1().toString()));
        } else {
          taskref->cache_filename = FileUtil::joinPaths(
              tempdir_,
              StringUtil::format("$0.rdd", cache_key.get()));
        }

        if (!taskref->cache_filename.empty() &&
            FileUtil::exists(taskref->cache_filename)) {
          stx::logDebug(
              "dproc",
              "Read RDD from cache: $0, key=$1",
              taskref->debug_name,
              cache_key.get());

          taskref->readCache();

          result->updateStatus([] (TaskStatus* status) {
            ++status->num_subtasks_completed;
          });

          taskref->finished = true;
          waiting = false;
          break;
        }

        auto parent_task = taskref;
        size_t numdeps = 0;
        for (const auto& dep : task->dependencies()) {
          RefPtr<LocalTaskContext> depref(new LocalTaskContext(app, dep.task_name, dep.params));
          parent_task->dependencies.emplace_back(depref);
          pipeline->tasks.emplace_back(depref);
          ++numdeps;
        }

        if (numdeps > 0) {
          result->updateStatus([numdeps] (TaskStatus* status) {
            status->num_subtasks_total += numdeps;
          });
        }

        waiting = false;
        break;
      }

      bool deps_finished = true;
      for (const auto& dep : taskref->dependencies) {
        if (dep->failed) {
          taskref->failed = true;
          taskref->finished = true;
          waiting = false;
        }

        if (!dep->finished) {
          deps_finished = false;
        }
      }

      if (!taskref->finished) {
        if (!deps_finished) {
          ++num_waiting;
          continue;
        }

        stx::logDebug(
            "dproc",
            "Computing RDD: $0",
            taskref->debug_name);

        taskref->running = true;

        while (pipeline->cur_tasks >= pipeline->max_tasks) {
          pipeline->wakeup.wait(lk);
        }

        ++pipeline->cur_tasks;
        work_tpool_.run(std::bind(
            &LocalScheduler::runTask,
            this,
            pipeline,
            taskref,
            result));

        waiting = false;
      }
    }

    stx::logDebug(
        "dproc",
        "Running local pipeline: $0",
        result->status().toString());

    if (waiting) {
      pipeline->wakeup.wait(lk);
    }

    while (pipeline->tasks.size() > 0 && pipeline->tasks.back()->finished) {
      pipeline->tasks.pop_back();
    }
  }
}

void LocalScheduler::runTask(
    RefPtr<LocalTaskPipeline> pipeline,
    RefPtr<LocalTaskContext> task,
    RefPtr<TaskResultFuture> result) {
  try {
    if (task->isCancelled()) {
      RAISE(kRuntimeError, "cancelled task");
    }

    bool from_cache = false;

    auto rdd = task->task_ref->getInstance();
    if (!task->cache_filename.empty() &&
        FileUtil::exists(task->cache_filename)) {
      stx::logDebug(
          "dproc",
          "Read RDD from cache: $0, key=$1",
          task->debug_name,
          rdd->cacheKeySHA1().get());

      task->readCache();
      from_cache = true;
    }

    if (!from_cache) {
      if (task->isCancelled()) {
        RAISE(kRuntimeError, "cancelled task");
      }

      rdd->compute(task.get());

      switch (rdd->storageLevel()) {

        case StorageLevel::MEMORY:
          break;

        case StorageLevel::DISK:
          auto cache_file = task->cache_filename;
          auto cache = rdd->encode();

          auto f = File::openFile(
              cache_file + "~",
              File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

          f.write(cache->data(), cache->size());

          FileUtil::mv(cache_file + "~", cache_file);

          task->task_ref = RefPtr<TaskRef>(
              new DiskTaskRef(
                  task->cache_filename,
                  task->task_factory,
                  rdd->contentType()));
          break;

      }
    }
  } catch (const std::exception& e) {
    task->failed = true;
    stx::logError("dproc", e, "error");
  }

  result->updateStatus([] (TaskStatus* status) {
    ++status->num_subtasks_completed;
  });

  std::unique_lock<std::mutex> lk(pipeline->mutex);
  task->finished = true;
  --pipeline->cur_tasks;
  lk.unlock();
  pipeline->wakeup.notify_all();
}

LocalScheduler::LocalTaskContext::LocalTaskContext(
    RefPtr<Application> app,
    const String& task_name,
    const Buffer& params) :
    task_factory([app, task_name, params] {
      return app->getTaskInstance(task_name, params);
    }),
    task_ref(new LiveTaskRef(task_factory())),
    debug_name(StringUtil::format("$0#$1", app->name(), task_name)),
    running(false),
    expanded(false),
    finished(false),
    failed(false),
    cancelled(false) {}

void LocalScheduler::LocalTaskContext::readCache() {
  task_ref = RefPtr<TaskRef>(
      new DiskTaskRef(
          cache_filename,
          task_factory,
          "application/octet-stream")); // FIXPAUL!
}

RefPtr<TaskRef> LocalScheduler::LocalTaskContext::getDependency(size_t index) {
  if (index >= dependencies.size()) {
    RAISEF(kIndexError, "invalid dependecy index: $0", index);
  }

  const auto& dep = dependencies[index];
  return dep->task_ref;
}

size_t LocalScheduler::LocalTaskContext::numDependencies() const {
  return dependencies.size();
}

void LocalScheduler::LocalTaskContext::cancel() {
  cancelled = true;
}

bool LocalScheduler::LocalTaskContext::isCancelled() const {
  return cancelled;
}

} // namespace dproc
