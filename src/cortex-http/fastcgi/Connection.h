// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <cortex-http/Api.h>
#include <cortex-http/fastcgi/RequestParser.h>
#include <cortex-http/fastcgi/Generator.h>
#include <cortex-base/net/EndPointWriter.h>
#include <cortex-http/HttpTransport.h>
#include <cortex-http/HttpHandler.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/TimeSpan.h>
#include <memory>
#include <unordered_map>
#include <list>

namespace cortex {
namespace http {

class HttpDateGenerator;
class HttpOutputCompressor;
class HttpChannel;
class HttpListener;

namespace fastcgi {

class HttpFastCgiChannel;
class HttpFastCgiTransport;

/**
 * @brief Implements a HTTP/1.1 transport connection.
 */
class CORTEX_HTTP_API Connection : public ::cortex::Connection {
  friend class HttpFastCgiTransport;
 public:
  Connection(EndPoint* endpoint,
             Executor* executor,
             const HttpHandler& handler,
             HttpDateGenerator* dateGenerator,
             HttpOutputCompressor* outputCompressor,
             size_t maxRequestUriLength,
             size_t maxRequestBodyLength,
             TimeSpan maxKeepAlive);
  ~Connection();

  HttpChannel* createChannel(int request);
  void removeChannel(int request);

  bool isPersistent() const noexcept { return persistent_; }
  void setPersistent(bool enable);

  // cortex::net::Connection overrides
  void onOpen() override;
  void onClose() override;
  void setInputBufferSize(size_t size) override;

 private:
  void parseFragment();

  void onFillable() override;
  void onFlushable() override;
  void onInterestFailure(const std::exception& error) override;
  void onResponseComplete(bool succeed);

  HttpListener* onCreateChannel(int request, bool keepAlive);
  void onUnknownPacket(int request, int record);
  void onAbortRequest(int request);

 private:
  HttpHandler handler_;
  size_t maxRequestUriLength_;
  size_t maxRequestBodyLength_;
  HttpDateGenerator* dateGenerator_;
  HttpOutputCompressor* outputCompressor_;
  TimeSpan maxKeepAlive_;
  Buffer inputBuffer_;
  size_t inputOffset_;
  bool persistent_;
  RequestParser parser_;

  std::unordered_map<int, std::unique_ptr<HttpFastCgiChannel>> channels_;

  EndPointWriter writer_;
  std::list<CompletionHandler> onComplete_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
