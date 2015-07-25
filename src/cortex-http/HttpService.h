// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/TimeSpan.h>
#include <cortex-base/DateTime.h> // BuiltinAssetHandler
#include <vector>
#include <unordered_map>

namespace cortex {

class Server;
class Connector;
class LocalConnector;
class InetConnector;
class Executor;
class WallClock;
class Scheduler;
class IPAddress;

namespace http {

class HttpRequest;
class HttpResponse;

/**
 * General purpose multi-handler HTTP Service API.
 *
 * @note HTTP/1 is always enabled by default.
 */
class CORTEX_HTTP_API HttpService {
 private:
  class InputListener;

 public:
  class Handler;
  class BuiltinAssetHandler;

  enum Protocol {
    HTTP1,
    FCGI,
  };

  HttpService();
  explicit HttpService(Protocol protocol);
  ~HttpService();

  /**
   * Configures this service to listen on TCP/IP using the given parameters.
   *
   * @param executor the executor service to run tasks on.
   * @param scheduler where to schedule timed tasks on.
   * @param clock The wall clock that may be used for timeout management.
   * @param readTimeout timespan indicating how long a connection may be idle for read.
   * @param writeTimeout timespan indicating how long a connection may be idle for write.
   * @param tcpFinTimeout Timespan to leave client sockets in FIN_WAIT2 state.
   *                      A value of 0 means to leave it at system default.
   * @param ipaddress the TCP/IP bind address.
   * @param port the TCP/IP port to listen on.
   * @param backlog the number of connections allowed to be queued in kernel.
   *
   */
  InetConnector* configureInet(Executor* executor,
                               Scheduler* scheduler,
                               WallClock* clock,
                               TimeSpan readTimeout,
                               TimeSpan writeTimeout,
                               TimeSpan tcpFinTimeout,
                               const IPAddress& ipaddress,
                               int port,
                               int backlog = 128);

  /** Configures a local connector. */
  LocalConnector* configureLocal();

  /** Registers a new @p handler. */
  void addHandler(Handler* handler);

  /** Removes given @p handler from the list of registered handlers. */
  void removeHandler(Handler* handler);

  /** Starts the internal server. */
  void start();

  /** Stops the internal server. */
  void stop();

 private:
  static Protocol getDefaultProtocol();
  void attachProtocol(Connector* connector);
  void attachHttp1(Connector* connector);
  void attachFCGI(Connector* connector);
  void handleRequest(HttpRequest* request, HttpResponse* response);
  void onAllDataRead(HttpRequest* request, HttpResponse* response);

  friend class ServiceInputListener;

 private:
  Protocol protocol_;
  Server* server_;
  LocalConnector* localConnector_;
  InetConnector* inetConnector_;
  std::vector<Handler*> handlers_;
};

/**
 * Interface for general purpose HTTP request handlers.
 */
class CORTEX_HTTP_API HttpService::Handler {
 public:
  /**
   * Attempts to handle the given request.
   *
   * @retval true the request is being handled.
   * @retval false the request is not being handled.
   */
  virtual bool handleRequest(HttpRequest* request, HttpResponse* response) = 0;
};

/**
 * Builtin Asset Handler for HttpService.
 */
class CORTEX_HTTP_API HttpService::BuiltinAssetHandler : public Handler {
 public:
  BuiltinAssetHandler();

  void addAsset(const std::string& path, const std::string& mimetype,
                const Buffer&& data);

  bool handleRequest(HttpRequest* request, HttpResponse* response) override;

 private:
  struct Asset {
    std::string mimetype;
    DateTime mtime;
    Buffer data;
  };

  std::unordered_map<std::string, Asset> assets_;
};

} // namespace http
} // namespace cortex
