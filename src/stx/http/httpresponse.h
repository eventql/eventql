/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_WEB_HTTPRESPONSE_H
#define _libstx_WEB_HTTPRESPONSE_H
#include <stx/UnixTime.h>
#include <stx/http/httpmessage.h>
#include <stx/http/httprequest.h>
#include <stx/http/status.h>
#include <string>

namespace stx {
namespace http {

class HTTPResponse : public HTTPMessage {
public:
  /**
   * Parse the provided http response string and return the parsed http response
   */
  static HTTPResponse parse(const std::string& str);

  HTTPResponse();

  void setStatus(int status_code, const std::string& status);
  void setStatus(const HTTPStatus& status);
  void setStatusCode(int code);
  void setStatusName(const std::string& status);

  void addCookie(
      const std::string& key,
      const std::string& value,
      const UnixTime& expire = UnixTime::epoch(),
      const std::string& path = "",
      const std::string& domain = "",
      bool secure = false,
      bool httponly = false);

  void populateFromRequest(const HTTPRequest& request);

  int statusCode() const;
  const std::string& statusName() const;

protected:
  int status_code_;
  std::string status_;
};

}
}
#endif
