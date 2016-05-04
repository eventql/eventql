/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_HTTP_CLIENTCONNECTION_H
#define _STX_HTTP_CLIENTCONNECTION_H
#include <memory>
#include <vector>
#include <eventql/util/http/httphandler.h>
#include <eventql/util/http/httpparser.h>
#include <eventql/util/http/httprequest.h>
#include <eventql/util/http/httpresponse.h>
#include <eventql/util/http/httpstats.h>
#include <eventql/util/net/inetaddr.h>
#include <eventql/util/net/tcpconnection.h>
#include <eventql/util/thread/taskscheduler.h>

namespace stx {
namespace http {
class HTTPResponseHandler;

class HTTPClientConnection {
public:
  static const size_t kMinBufferSize = 4096;

  HTTPClientConnection(
      std::unique_ptr<net::TCPConnection> conn,
      TaskScheduler* scheduler,
      HTTPClientStats* stats);

  ~HTTPClientConnection();

  HTTPClientConnection(const HTTPClientConnection& other) = delete;
  HTTPClientConnection& operator=(const HTTPClientConnection& other) = delete;

  void executeRequest(
      const HTTPRequest& request,
      HTTPResponseHandler* response_handler);

  Wakeup* onReady();

  bool isIdle() const;

protected:

  enum kHTTPClientConnectionState {
    S_CONN_IDLE,
    S_CONN_BUSY,
    S_CONN_CLOSED
  };

  void read();
  void write();
  void awaitRead();
  void awaitWrite();
  void close();
  void keepalive();

  void error(const std::exception& e);

  std::unique_ptr<net::TCPConnection> conn_;
  TaskScheduler* scheduler_;
  kHTTPClientConnectionState state_;
  HTTPParser parser_;
  Buffer buf_;
  std::mutex mutex_;
  HTTPResponseHandler* cur_handler_;
  Wakeup on_ready_;
  bool keepalive_;
  HTTPClientStats* stats_;
};

}
}
#endif
