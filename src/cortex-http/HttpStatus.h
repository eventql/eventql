// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <string>

namespace cortex {
namespace http {

//! \addtogroup http
//@{

/**
 * HTTP status code.
 *
 * \see http://www.iana.org/assignments/http-status-codes/http-status-codes.xml
 */
enum class HttpStatus  // {{{
{
  Undefined = 0,

  // informational
  ContinueRequest = 100,
  SwitchingProtocols = 101,
  Processing = 102,  // WebDAV, RFC 2518

  // successful
  Ok = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoriativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,

  // redirection
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  MovedTemporarily = Found,
  NotModified = 304,
  TemporaryRedirect = 307,  // since HTTP/1.1
  PermanentRedirect = 308,  // Internet-Draft

  // client error
  BadRequest = 400,
  Unauthorized = 401,
  PaymentRequired = 402,  // reserved for future use
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  ProxyAuthenticationRequired = 407,
  RequestTimeout = 408,
  Conflict = 409,
  Gone = 410,
  LengthRequired = 411,
  PreconditionFailed = 412,
  PayloadTooLarge = 413,
  RequestUriTooLong = 414,
  UnsupportedMediaType = 415,
  RequestedRangeNotSatisfiable = 416,
  ExpectationFailed = 417,
  ThereAreTooManyConnectionsFromYourIP = 421,
  UnprocessableEntity = 422,
  Locked = 423,
  FailedDependency = 424,
  UnorderedCollection = 425,
  UpgradeRequired = 426,
  PreconditionRequired = 428,         // RFC 6585
  TooManyRequests = 429,              // RFC 6585
  RequestHeaderFieldsTooLarge = 431,  // RFC 6585
  NoResponse = 444,  // nginx ("Used in Nginx logs to indicate that the server
                     // has returned no information to the client and closed the
                     // connection")
  Hangup = 499,  // Used in Nginx to indicate that the client has aborted the
                 // connection before the server could serve the response.

  // server error
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HttpVersionNotSupported = 505,
  VariantAlsoNegotiates = 506,         // RFC 2295
  InsufficientStorage = 507,           // WebDAV, RFC 4918
  LoopDetected = 508,                  // WebDAV, RFC 5842
  BandwidthExceeded = 509,             // Apache
  NotExtended = 510,                   // RFC 2774
  NetworkAuthenticationRequired = 511  // RFC 6585
};
// }}}

inline bool operator!(HttpStatus st) {
  //.
  return st == HttpStatus::Undefined;
}

/** Retrieves the human readable text of the HTTP status @p code. */
CORTEX_HTTP_API const std::string& to_string(HttpStatus code);

/** Tests whether given status @p code MUST NOT have a message body. */
CORTEX_HTTP_API bool isContentForbidden(HttpStatus code);

/** Tests whether given status @p code is informatiional (1xx). */
CORTEX_HTTP_API bool isInformational(HttpStatus code);

/** Tests whether given status @p code is successful (2xx). */
CORTEX_HTTP_API bool isSuccess(HttpStatus code);

/** Tests whether given status @p code is a redirect (3xx). */
CORTEX_HTTP_API bool isRedirect(HttpStatus code);

/** Tests whether given status @p code is a client error (4xx). */
CORTEX_HTTP_API bool isClientError(HttpStatus code);

/** Tests whether given status @p code is a server error (5xx). */
CORTEX_HTTP_API bool isServerError(HttpStatus code);
//@}

// {{{ inlines
inline bool isInformational(HttpStatus code) {
  return static_cast<int>(code) / 100 == 1;
}

inline bool isSuccess(HttpStatus code) {
  return static_cast<int>(code) / 100 == 2;
}

inline bool isRedirect(HttpStatus code) {
  return static_cast<int>(code) / 100 == 3;
}

inline bool isClientError(HttpStatus code) {
  return static_cast<int>(code) / 100 == 4;
}

inline bool isServerError(HttpStatus code) {
  return static_cast<int>(code) / 100 == 5;
}

inline bool isContentForbidden(HttpStatus code) {
  switch (code) {
    case /*100*/ HttpStatus::ContinueRequest:
    case /*101*/ HttpStatus::SwitchingProtocols:
    case /*204*/ HttpStatus::NoContent:
    case /*205*/ HttpStatus::ResetContent:
    case /*304*/ HttpStatus::NotModified:
      return true;
    default:
      return false;
  }
}
// }}}

}  // namespace http
}  // namespace cortex
