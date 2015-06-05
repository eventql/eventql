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
#include <fnord-base/io/file.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/logging.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-dproc/LocalScheduler.h>

namespace fnord {
namespace dproc {

LocalScheduler::LocalScheduler(
    const String& tempdir /* = "/tmp" */,
    size_t max_threads /* = 8 */,
    size_t max_requests /* = 32 */) :
    tempdir_(tempdir),
    tpool_(max_threads),
    req_tpool_(max_requests) {}

void LocalScheduler::start() {
  req_tpool_.start();
  tpool_.start();
}

void LocalScheduler::stop() {
  req_tpool_.stop();
  tpool_.stop();
}

RefPtr<TaskResultFuture> LocalScheduler::run(
    RefPtr<Application> app,
    const TaskSpec& task) {
  RefPtr<TaskResultFuture> result(new TaskResultFuture());

  try {
    auto instance = mkRef(
        new LocalTaskRef(
            app,
            task.task_name(),
            Buffer(task.params().data(), task.params().size())));

    req_tpool_.run([this, app, result, instance] () {
      try {
        LocalTaskPipeline pipeline;
        pipeline.tasks.push_back(instance);
        runPipeline(app.get(), &pipeline, result);

        if (instance->failed) {
          RAISE(kRuntimeError, "task failed");
        }

        result->returnResult(instance->task);
      } catch (const StandardException& e) {
        fnord::logError("dproc.scheduler", e, "task failed");
        result->returnError(e);
      }
    });
  } catch (const StandardException& e) {
    fnord::logError("dproc.scheduler", e, "task failed");
    result->returnError(e);
  }

  return result;
}

void LocalScheduler::runPipeline(
    Application* app,
    LocalTaskPipeline* pipeline,
    RefPtr<TaskResultFuture> result) {
  fnord::logDebug(
      "fnord.dproc",
      "Starting local pipeline id=$0 tasks=$1",
      (void*) pipeline,
      pipeline->tasks.size());

  std::unique_lock<std::mutex> lk(pipeline->mutex);
  result->updateStatus([&pipeline] (TaskStatus* status) {
    status->num_subtasks_total = pipeline->tasks.size();
  });

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

        auto cache_key = taskref->task->cacheKeySHA1();
        if (!cache_key.isEmpty()) {
          taskref->cache_filename = FileUtil::joinPaths(
              tempdir_,
              StringUtil::format("$0.rdd", cache_key.get()));
        }

        auto cached =
            !cache_key.isEmpty() &&
            FileUtil::exists(taskref->cache_filename);

        if (cached) {
          auto cache_file = File::openFile(
              taskref->cache_filename,
              File::O_READ);

          Buffer cache_hdr(sizeof(uint64_t));
          cache_file.read(&cache_hdr);
          util::BinaryMessageReader reader(cache_hdr.data(), cache_hdr.size());
          auto cache_version = *reader.readUInt64();
          auto cache_size = cache_file.size() - cache_hdr.size();

          CachedTask cache {
            .version = cache_version,
            .data = new io::MmappedFile(
                  std::move(cache_file),
                  cache_hdr.size(),
                  cache_size)
          };

          if (taskref->task->unpersist(cache)) {
            fnord::logDebug(
                "fnord.dproc",
                "Read RDD from cache: $0, key=$1, version=$2",
                taskref->debug_name,
                cache_key.get(),
                cache_version);

            result->updateStatus([&pipeline] (TaskStatus* status) {
              ++status->num_subtasks_completed;
            });

            taskref->finished = true;
            waiting = false;
            break;
          }
        }

        auto parent_task = taskref;
        size_t numdeps = 0;
        for (const auto& dep : taskref->task->dependencies()) {
          RefPtr<LocalTaskRef> depref(new LocalTaskRef(app, dep.task_name, dep.params));
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

        auto ckey = taskref->task->cacheKeySHA1();
        fnord::logDebug(
            "fnord.dproc",
            "Computing RDD: $0, key=$1",
            taskref->debug_name,
            ckey.isEmpty() ? "<nil>" : ckey.get());

        taskref->running = true;
        tpool_.run(std::bind(
            &LocalScheduler::runTask,
            this,
            pipeline,
            taskref,
            result));

        waiting = false;
      }
    }

    fnord::logDebug(
        "fnord.dproc",
        "Running local pipeline id=$0: $1",
        (void*) pipeline,
        result->status().toString());

    if (waiting) {
      pipeline->wakeup.wait(lk);
    }

    while (pipeline->tasks.size() > 0 && pipeline->tasks.back()->finished) {
      pipeline->tasks.pop_back();
    }
  }

  fnord::logDebug(
      "fnord.dproc",
      "Completed local pipeline id=$0",
      (void*) pipeline);
}

void LocalScheduler::runTask(
    LocalTaskPipeline* pipeline,
    RefPtr<LocalTaskRef> task,
    RefPtr<TaskResultFuture> result) {

  try {
    task->task->compute(task.get());

    if (!task->cache_filename.empty()) {
      auto cache = task->task->persist();
      auto cache_file = task->cache_filename;

      {
        auto f = File::openFile(
            cache_file + "~",
            File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

        util::BinaryMessageWriter hdr;
        hdr.appendUInt64(cache.version);
        f.write(hdr.data(), hdr.size());
        f.write(cache.data->data(), cache.data->size());
      }

      FileUtil::mv(cache_file + "~", cache_file);
    }
  } catch (const std::exception& e) {
    task->failed = true;
    fnord::logError("fnord.dproc", e, "error");
  }

  result->updateStatus([&pipeline] (TaskStatus* status) {
    ++status->num_subtasks_completed;
  });

  std::unique_lock<std::mutex> lk(pipeline->mutex);
  task->finished = true;
  lk.unlock();
  pipeline->wakeup.notify_all();
}

LocalScheduler::LocalTaskRef::LocalTaskRef(
    RefPtr<Application> app,
    const String& task_name,
    const Buffer& params) :
    task(app->getTaskInstance(task_name, params)),
    debug_name(StringUtil::format("$0#$1", app->name(), task_name)),
    running(false),
    expanded(false),
    finished(false),
    failed(false) {}

RefPtr<Task> LocalScheduler::LocalTaskRef::getDependency(size_t index) {
  if (index >= dependencies.size()) {
    RAISEF(kIndexError, "invalid dependecy index: $0", index);
  }

  const auto& dep = dependencies[index];
  return dep->task;
}

size_t LocalScheduler::LocalTaskRef::numDependencies() const {
  return dependencies.size();
}

} // namespace dproc
} // namespace fnord
