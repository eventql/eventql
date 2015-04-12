/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-eventdb/EventDBServlet.h"

namespace fnord {
namespace eventdb {

void EventDBServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/insert")) {
      return insertRecord(req, res, &uri);
    }

    res->setStatus(fnord::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.getMessage()));
  }
}

void EventDBServlet::insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  const auto& body = req->body();
  fnord::iputs("req: $0 -> $1 bytes", table, body.size());

  res->setStatus(http::kStatusCreated);
}

}
}
