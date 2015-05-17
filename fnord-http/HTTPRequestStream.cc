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
#include <fnord-http/HTTPRequestStream.h>

namespace fnord {
namespace http {

HTTPRequestStream::HTTPRequestStream(
    const HTTPRequest& req,
    HTTPServerConnection* conn) :
    req_(req),
    conn_(conn) {}

const HTTPRequest& HTTPRequestStream::request() const {
  return req_;
}

//void HTTPRequestStream::readBody() {
//}

}
}
