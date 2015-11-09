// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/net/Server.h>
#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/net/LocalConnector.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/executor/ThreadedExecutor.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/Buffer.h>
#include <algorithm>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <pthread.h>

static void setCpuAffinity(int cpu) {
#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
  cpu_set_t set;

  CPU_ZERO(&set);
  CPU_SET(cpu, &set);

  int rv = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
  if (rv < 0) {
    perror("pthread_setaffinity_np");
  }
#endif
}

static int processor_count() {
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
  int rv = sysconf(_SC_NPROCESSORS_ONLN);
  return rv < 0 ? 1 : rv;
#else
  return 1;
#endif
}

/**
 * Threaded CPU-core affine multiplexed I/O network service.
 */
class ThreadedScheduler { // {{{
 public:
  /**
   * Thread Worker API.
   */
  struct Worker {
   public:
    virtual ~Worker() {}

    virtual cortex::Scheduler* scheduler() = 0;
  };

  /**
   * Worker factory API.
   */
  class WorkerFactory {
   public:
    virtual std::unique_ptr<Worker> createWorker() = 0;
  };

  explicit ThreadedScheduler(int thread_count, WorkerFactory* workerFactory);
  ~ThreadedScheduler();

  /** iterates through each I/O worker's I/O scheduler. */
  void each(std::function<void(Worker*)>&& itr);

  /** Starts the server as well as the I/O worker threads. */
  void start();

  /** Stops the server as well as the I/O worker threads. */
  void stop();

  cortex::Server* server() CORTEX_NOEXCEPT { return &server_; }

 private:
  cortex::ThreadedExecutor threadedExecutor_;
  cortex::Server server_;
  std::vector<Worker*> workers_;
  WorkerFactory* workerFactory_;
};

ThreadedScheduler::ThreadedScheduler(int thread_count,
                                   WorkerFactory* workerFactory)
    : threadedExecutor_(),
      server_(),
      workers_(),
      workerFactory_(workerFactory) {

  for (size_t i = 0; i < thread_count; ++i) {
    workers_.push_back(workerFactory->createWorker().release());
  }
}

ThreadedScheduler::~ThreadedScheduler() {
  for (Worker* worker: workers_)
    delete worker;
}

void ThreadedScheduler::each(std::function<void(Worker*)>&& itr) {
  std::for_each(workers_.begin(), workers_.end(), itr);
  // for (Worker* worker: workers_) {
  //   itr(worker);
  // }
}

void ThreadedScheduler::start() {
  server_.start();

  for (size_t i = 0; i < workers_.size(); ++i) {
    cortex::Scheduler* scheduler = workers_[i]->scheduler();
    char name[16];
    snprintf(name, sizeof(name), "cortex-io/%zu", i);

    threadedExecutor_.execute(name, [i, name, scheduler]() {
      printf("executing: %s\n", name);
      setCpuAffinity(i);
      scheduler->runLoop();
    });
  }
}

void ThreadedScheduler::stop() {
  for (Worker* worker: workers_)
    worker->scheduler()->breakLoop();

  server_.stop();

  threadedExecutor_.joinAll();
}
// }}}
class MyWorker : public ThreadedScheduler::Worker { // {{{
 public:
  MyWorker();
  ~MyWorker();

  cortex::Scheduler* scheduler() override { return scheduler_.get(); }
  cortex::Executor* executor() { return scheduler(); }
  cortex::WallClock* clock() { return clock_; }

 private:
  std::unique_ptr<cortex::Scheduler> scheduler_;
  cortex::WallClock* clock_;
  int i_;
};

static int wi = 0;
MyWorker::MyWorker()
    : scheduler_(new cortex::NativeScheduler()),
      clock_(cortex::WallClock::monotonic()),
      i_(wi++) {
  printf("Creating worker %d\n", i_);
}

MyWorker::~MyWorker() {
  printf("Terminating worker %d\n", i_);
}
// }}}
class MyWorkerFactory : public ThreadedScheduler::WorkerFactory { // {{{
 public:
  std::unique_ptr<ThreadedScheduler::Worker> createWorker() override;
};

std::unique_ptr<ThreadedScheduler::Worker> MyWorkerFactory::createWorker() {
  return std::unique_ptr<ThreadedScheduler::Worker>(new MyWorker());
}
// }}}

static std::condition_variable quitCondition;

class EchoConnection : public cortex::Connection { // {{{
 public:
  EchoConnection(cortex::EndPoint* endpoint,
                 cortex::Executor* executor)
      : cortex::Connection(endpoint, executor) {
    printf("EchoConnection()\n");
  }

  ~EchoConnection() {
    printf("~EchoConnection()\n");
  }

  void onOpen() override {
    Connection::onOpen();
    wantFill();
  }

  void onFillable() override {
    cortex::Buffer data;
    endpoint()->fill(&data);

    printf("echo[%s]: %s", getThreadName().c_str(), data.c_str());

    if (data == "shutdown\r\n")
      quitCondition.notify_one();
    else {
      endpoint()->flush(data);
      close();
    }
  }

  static std::string getThreadName() {
    pthread_t tid = pthread_self();
    char name[16] = { 0 };
    pthread_getname_np(tid, name, sizeof(name));
    return name;
  }

  void onFlushable() override {
    //.
  }
};
// }}}
class EchoFactory : public cortex::ConnectionFactory { // {{{
 public:
  EchoFactory() : cortex::ConnectionFactory("echo") {}

  EchoConnection* create(cortex::Connector* connector,
                         cortex::EndPoint* endpoint) override {
    return static_cast<EchoConnection*>(configure(
        new EchoConnection(endpoint, connector->executor()),
        connector));
    ;
  }
};
// }}}

int main(int argc, char* argv[]) {
  MyWorkerFactory workerFactory;
  ThreadedScheduler srv(processor_count(), &workerFactory);

  // create a SO_REUSEPORT-enabled connector for each worker
  srv.each([&srv](ThreadedScheduler::Worker* worker) {
    MyWorker* ew = static_cast<MyWorker*>(worker);

    auto inet = std::unique_ptr<cortex::InetConnector>(new cortex::InetConnector(
        "echo", ew->executor(), ew->scheduler(), ew->clock(),
        cortex::TimeSpan::fromSeconds(30),
        cortex::TimeSpan::fromSeconds(30),
        cortex::TimeSpan::Zero,
        &cortex::logAndPass,
        cortex::IPAddress("0.0.0.0"), 3000, 128, true, true));

    inet->setBlocking(false);
    inet->setQuickAck(true);
    inet->setDeferAccept(true);
    inet->setMultiAcceptCount(1);
    inet->addConnectionFactory(std::make_shared<EchoFactory>());

    srv.server()->addConnector(std::move(inet));
  });

  srv.start();

  {
    std::mutex quitMutex;
    std::unique_lock<decltype(quitMutex)> lock(quitMutex);
    quitCondition.wait(lock);
  }

  srv.stop();

  return 0;
}
