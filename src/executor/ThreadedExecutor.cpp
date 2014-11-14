// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/executor/ThreadedExecutor.h>
#include <xzero/sysconfig.h>
#include <memory>
#include <algorithm>
#include <limits>
#include <pthread.h>

namespace xzero {

#if 0 //!defined(NDEBUG)
static std::mutex m;
#define TRACE(msg...)  do { \
    m.lock(); \
    printf("ThreadedExecutor: " msg); \
    printf("\n"); \
    m.unlock(); \
  } while (0);
#else
#define TRACE(msg...) do { } while (0)
#endif

ThreadedExecutor::ThreadedExecutor() : threads_() {
}

ThreadedExecutor::~ThreadedExecutor() {
  joinAll();
}

static std::string getThreadName(pthread_t tid) {
  char name[16];
  name[0] = '\0';
  pthread_getname_np(tid, name, sizeof(name));
  return name;
}

void ThreadedExecutor::joinAll() {
  for (;;) {
    pthread_t tid = 0;
    {
      TRACE("joinAll: getting lock for getting TID");
      std::lock_guard<std::mutex> lock(mutex_);
      if (threads_.empty())
        break;

      tid = threads_.front();
      threads_.pop_front();
    }
    TRACE("joinAll: join(%lu) %s", tid, getThreadName(tid).c_str());
    pthread_join(tid, nullptr);
  }
  TRACE("joinAll: done");
}

void* ThreadedExecutor::launchme(void* ptr) {
  TRACE("launchme[%d](%p) enter", pthread_self(), ptr);
  std::unique_ptr<Executor::Task> task(reinterpret_cast<Executor::Task*>(ptr));
  (*task)();
  TRACE("launchme[%d](%p) leave", pthread_self(), ptr);
  return nullptr;
}

void ThreadedExecutor::execute(const std::string& name, Task&& task) {
  pthread_t tid;
  auto runner = [this, task]() {
    safeCall(task);
  };
  pthread_create(&tid, NULL, &launchme, new Task{std::move(runner)});

#if !defined(__APPLE__)
  // OS/x doesn't support setting thread names for other threads
  // TODO: pass thread name to target thread and set it inside
  pthread_setname_np(tid, name.c_str());
#endif

  std::lock_guard<std::mutex> lock(mutex_);
  threads_.push_back(tid);
}

void ThreadedExecutor::execute(Task&& task) {
  pthread_t tid = 0;
  //pthread_create(&tid, NULL, &launchme, new Task{std::move(task)});
  pthread_create(&tid, NULL, &launchme, new Task([this, task]{
    pthread_t tid = pthread_self();
    safeCall(task);
    {
      TRACE("task %s finished. getting lock for cleanup", getThreadName(tid).c_str());
      std::lock_guard<std::mutex> lock(mutex_);
      pthread_detach(tid);
      auto i = std::find(threads_.begin(), threads_.end(), tid);
      if (i != threads_.end()) {
        threads_.erase(i);
      }
    }
  }));
  std::lock_guard<std::mutex> lock(mutex_);
  threads_.push_back(tid);
}

size_t ThreadedExecutor::maxConcurrency() const XZERO_NOEXCEPT {
  return std::numeric_limits<size_t>::max();
}

std::string ThreadedExecutor::toString() const {
  char buf[32];
  snprintf(buf, sizeof(buf), "ThreadedExecutor@%p", this);
  return buf;
}

} // namespace xzero
