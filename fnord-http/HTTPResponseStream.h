/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_HTTP_HTTPRESPONSESTREAM_H
#define _FNORD_HTTP_HTTPRESPONSESTREAM_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-http/httpresponse.h>
#include <fnord-http/httpserverconnection.h>

namespace fnord {
namespace http {

class HTTPResponseStream : public RefCounted {
public:

  HTTPResponseStream(HTTPServerConnection* conn);

  void writeResponse(const HTTPResponse& resp);
  void writeBodyChunk(const Buffer& buf);
  void finishResponse();

  bool isOutputStarted() const;

protected:

  void onCallbackCompleted();
  void onStateChanged();

  mutable std::mutex mutex_;
  HTTPServerConnection* conn_;
  bool callback_running_;
  bool headers_written_;
  bool response_finished_;
  Buffer buf_;
};

}
}
#endif
