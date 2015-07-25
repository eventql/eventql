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
#include <cortex-http/HeaderFieldList.h>
#include <cortex-base/Buffer.h>
#include <unordered_map>
#include <functional>
#include <utility>
#include <string>
#include <list>

namespace cortex {
namespace http {

class HttpListener;

namespace fastcgi {

/**
 * Parses a client FastCGI request stream.
 *
 */
class CORTEX_HTTP_API RequestParser {
 private:
  struct StreamState : public CgiParamStreamReader { // {{{
    HttpListener* listener;
    size_t totalBytesReceived;
    bool paramsFullyReceived;
    bool contentFullyReceived;
    HeaderFieldList params;
    HeaderFieldList headers;

    // keeps the body at least as long as params haven't been fully parsed.
    Buffer body;

    StreamState();
    ~StreamState();

    void reset();

    void onParam(const char *name, size_t nameLen,
                 const char *value, size_t valueLen) override;
  }; // }}}

 public:
  RequestParser(
      std::function<HttpListener*(int requestId, bool keepAlive)> onCreateChannel,
      std::function<void(int requestId, int recordId)> onUnknownPacket,
      std::function<void(int requestId)> onAbortRequest);

  void reset();

  StreamState* registerStreamState(int requestId);
  StreamState* getStream(int requestId);
  void removeStreamState(int requestId);

  size_t parseFragment(const BufferRef& chunk);
  size_t parseFragment(const Record& record);

  template<typename RecordType, typename... Args>
  size_t parseFragment(Args... args) {
    return parseFragment(RecordType(args...));
  }

 protected:
  void process(const fastcgi::Record* record);
  void beginRequest(const fastcgi::BeginRequestRecord* record);
  void streamParams(const fastcgi::Record* record);
  void streamStdIn(const fastcgi::Record* record);
  void streamStdOut(const fastcgi::Record* record);
  void streamStdErr(const fastcgi::Record* record);
  void streamData(const fastcgi::Record* record);
  void abortRequest(const fastcgi::AbortRequestRecord* record);

 protected:
  std::function<HttpListener*(int requestId, bool keepAlive)> onCreateChannel_;
  std::function<void(int requestId, int recordId)> onUnknownPacket_;
  std::function<void(int requestId)> onAbortRequest_;

  std::unordered_map<int, StreamState> streams_;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex
