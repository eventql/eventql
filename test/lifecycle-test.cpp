#include <xzero/net/Server.h>
#include <xzero/net/Connector.h>
#include <xzero/net/LocalConnector.h>
#include <xzero/net/InetConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/net/IPAddress.h>
#include <xzero/executor/DirectExecutor.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/v1/HttpConnectionFactory.h>
#include <xzero/support/libev/LibevScheduler.h>
#include <xzero/support/libev/LibevClock.h>
#include <ev++.h>

using xzero::Buffer;
using xzero::IPAddress;

xzero::InetConnector* createInet(xzero::Server& server) {
  xzero::InetConnector* inetConnector = nullptr; // TODO server.addConnector<xzero::InetConnector>();

  inetConnector->open(IPAddress("0.0.0.0"), 8089, 128, true, true);

  inetConnector->setBlocking(true);
  inetConnector->setCloseOnExec(true);
  inetConnector->setQuickAck(true);
  inetConnector->setDeferAccept(true);
  inetConnector->setMultiAcceptCount(1);

  return inetConnector;
}

int main() {
  ev::loop_ref loop = ev::default_loop(0);
  xzero::support::LibevClock clock(loop);
  std::unique_ptr<xzero::Executor> taskExecutor(new xzero::DirectExecutor(false));
  std::unique_ptr<xzero::Scheduler> scheduler(new xzero::support::LibevScheduler(loop));

  xzero::Server server;
  auto inet = createInet(server);
  auto local = server.addConnector<xzero::LocalConnector>();

  auto http = std::make_shared<xzero::http1::HttpConnectionFactory>();
  local->addConnectionFactory(http);
  inet->addConnectionFactory(http);

  http->setClock(&clock);

  http->setHandler([](xzero::HttpRequest* request,
                      xzero::HttpResponse* response) {
    printf("Incoming request. '%s' '%s'\n", request->method().c_str(),
           request->path().c_str());

    response->setStatus(xzero::HttpStatus::Ok);
    response->addHeader("Foo", "breach");
    response->output()->write("Hello, World!\n");
    response->completed();
  });

  server.start();

  std::shared_ptr<xzero::LocalEndPoint> ep = local->createClient(
      "GET /fnord.txt HTTP/1.1\r\n"
      "Connection: Keep-Alive\r\n"
      "Host: localhost\r\n"
      "\r\n"
      "GET /fnord.txt HTTP/1.1\r\n"
      "Connection: close\r\n"
      "Host: localhost\r\n"
      "\r\n"
  );

  Buffer output = ep->output();
  printf("======================================================\n");
  printf("%s\n", output.c_str());
  printf("======================================================\n");

  ep->close();

  server.stop();
}
