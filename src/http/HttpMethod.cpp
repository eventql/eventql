// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpMethod.h>

namespace xzero {

#define SRET(slit) { static std::string val(slit); return val; }

std::string to_string(HttpMethod value) {
  switch (value) {
    case HttpMethod::UNKNOWN_METHOD: SRET("UNKNOWN_METHOD");
    case HttpMethod::OPTIONS: SRET("OPTIONS");
    case HttpMethod::GET: SRET("GET");
    case HttpMethod::HEAD: SRET("HEAD");
    case HttpMethod::POST: SRET("POST");
    case HttpMethod::PUT: SRET("PUT");
    case HttpMethod::DELETE: SRET("DELETE");
    case HttpMethod::TRACE: SRET("TRACE");
    case HttpMethod::CONNECT: SRET("CONNECT");
    default: SRET("UNDEFINED_METHOD");
  }
}

#define SCMP(lhs, rhs, result) \
  (lhs) == (rhs) ? (result) : (HttpMethod::UNKNOWN_METHOD)

HttpMethod to_method(const std::string& value) {
  if (value.empty())
    return HttpMethod::UNKNOWN_METHOD;

  switch (value[0]) {
    case 'O': return SCMP(value, "OPTIONS", HttpMethod::OPTIONS);
    case 'G': return SCMP(value, "GET", HttpMethod::GET);
    case 'H': return SCMP(value, "HEAD", HttpMethod::HEAD);
    case 'D': return SCMP(value, "DELETE", HttpMethod::DELETE);
    case 'T': return SCMP(value, "TRACE", HttpMethod::TRACE);
    case 'C': return SCMP(value, "CONNECT", HttpMethod::CONNECT);
    case 'P': return value == "POST"
                ? HttpMethod::POST
                : value == "PUT"
                  ? HttpMethod::PUT
                  : HttpMethod::UNKNOWN_METHOD;
    default:
      return HttpMethod::UNKNOWN_METHOD;
  }
}

} // namespace xxzero
