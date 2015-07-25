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
#include <cortex-http/fastcgi/bits.h>
#include <cortex-http/http1/Parser.h>
#include <cortex-base/Buffer.h>
#include <unordered_map>
#include <functional>
#include <utility>

namespace cortex {
namespace http {

class HttpListener;

namespace fastcgi {

/**
 * Parses a client FastCGI stream (upstream & downstream side).
 */
class CORTEX_HTTP_API ResponseParser {
 private:
  struct StreamState { // {{{
    HttpListener* listener;
    size_t totalBytesReceived;
    bool contentFullyReceived;
    http::http1::Parser http1Parser;

    StreamState();
    ~StreamState();
    void reset();
    void setListener(HttpListener* listener);
  }; // }}}

 public:
  ResponseParser(
      std::function<HttpListener*(int requestId)> onCreateChannel,
      std::function<void(int requestId, int recordId)> onUnknownPacket,
      std::function<void(int requestId, const BufferRef&)> onStdErr);

  void reset();

  StreamState& registerStreamState(int requestId);
  void removeStreamState(int requestId);

  size_t parseFragment(const BufferRef& chunk);
  size_t parseFragment(const Record& record);

  template<typename RecordType, typename... Args>
  size_t parseFragment(Args... args) {
    return parseFragment(RecordType(args...));
  }

 protected:
  StreamState& getStream(int requestId);
  void process(const fastcgi::Record* record);
  void streamStdOut(const fastcgi::Record* record);
  void streamStdErr(const fastcgi::Record* record);

 protected:
  std::function<HttpListener*(int requestId)> onCreateChannel_;
  std::function<void(int requestId, int recordId)> onUnknownPacket_;
  std::function<void(int requestId)> onAbortRequest_;
  std::function<void(int requestId, const BufferRef& content)> onStdErr_;

  std::unordered_map<int, StreamState> streams_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
