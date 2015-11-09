// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-base/Buffer.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/ConnectionFactory.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/net/LocalConnector.h>
#include <cortex-base/net/InetConnector.h>
#include <cortex-base/net/SslConnector.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/WallClock.h>
#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/executor/NativeScheduler.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/logging.h>

using cortex::Buffer;
using cortex::IPAddress;

class EchoConnection : public cortex::Connection { // {{{
 public:
  EchoConnection(cortex::EndPoint* endpoint,
                 cortex::Executor* executor)
      : cortex::Connection(endpoint, executor) {
  }

  ~EchoConnection() {
  }

  void onOpen() override {
    Connection::onOpen();
    wantFill();
  }

  void onFillable() override {
    Buffer data;
    endpoint()->fill(&data);

    executor()->execute([this, data]() {
      printf("echo: %s", data.c_str());
      endpoint()->flush(data);
      close();
    });
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
std::unique_ptr<cortex::InetConnector> createInetConnector( // {{{
    const std::string& name, int port, cortex::Executor* executor,
    cortex::Scheduler* scheduler, cortex::WallClock* clock) {

  std::unique_ptr<cortex::InetConnector> inetConnector(
      new cortex::InetConnector(name, executor, scheduler, clock,
        cortex::TimeSpan::fromSeconds(30),
        cortex::TimeSpan::fromSeconds(30),
        cortex::TimeSpan::Zero,
        &cortex::logAndPass));

  inetConnector->open(IPAddress("0.0.0.0"), port, 128, true, true);
  inetConnector->setBlocking(false);
  inetConnector->setCloseOnExec(true);
  inetConnector->setQuickAck(true);
  inetConnector->setDeferAccept(true);
  inetConnector->setMultiAcceptCount(1);

  return inetConnector;
}
// }}}
std::unique_ptr<cortex::SslConnector> createSslConnector( // {{{
    const std::string& name, int port, cortex::Executor* executor,
    cortex::Scheduler* scheduler, cortex::WallClock* clock) {

  std::unique_ptr<cortex::SslConnector> connector(
      new cortex::SslConnector(name, executor, scheduler, clock,
                              cortex::TimeSpan::fromSeconds(30),
                              cortex::TimeSpan::fromSeconds(30),
                              cortex::TimeSpan::Zero,
                              &cortex::logAndPass,
                              IPAddress("0.0.0.0"), port, 128, true, true));

  connector->addContext("../../server.crt", "../../server.key");

  return connector;
}
// }}}


int main(int argc, const char* argv[]) {
  try {
    cortex::DirectExecutor executor(false);
    cortex::NativeScheduler scheduler;
    cortex::WallClock* clock = cortex::WallClock::monotonic();
    cortex::Server server;

    auto localConnector = server.addConnector<cortex::LocalConnector>(&executor);
    localConnector->addConnectionFactory<EchoFactory>();

    auto inetConnector = createInetConnector("inet", 3000, &executor, &scheduler,
                                             clock);
    inetConnector->addConnectionFactory<EchoFactory>();
    server.addConnector(std::move(inetConnector));

    auto sslConnector = createSslConnector("ssl", 3443, &executor, &scheduler,
                                           clock);
    sslConnector->addConnectionFactory<EchoFactory>();
    server.addConnector(std::move(sslConnector));

    server.start();

    auto ep = localConnector->createClient("Hello, World!\n");
    printf("local result: %s", ep->output().c_str());

    scheduler.runLoop();

    server.stop();
  } catch (const std::exception& e) {
    cortex::logAndPass(e);
  }

  return 0;
}
