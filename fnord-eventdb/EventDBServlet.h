/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_EVENTDBSERVLET_H
#define _FNORD_EVENTDB_EVENTDBSERVLET_H
#include "fnord-http/httpservice.h"
#include "fnord-json/json.h"
#include <fnord-eventdb/TableRepository.h>

namespace fnord {
namespace eventdb {

class EventDBServlet : public fnord::http::HTTPService {
public:
  enum class ResponseFormat {
    JSON,
    CSV
  };

  EventDBServlet(TableRepository* tables);

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void commitTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  TableRepository* tables_;
  //ResponseFormat formatFromString(const String& format);
};

}
}
#endif
