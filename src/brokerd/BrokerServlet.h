/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_BROKER_BROKERSERVLET_H
#define _FNORD_BROKER_BROKERSERVLET_H
#include "stx/http/httpservice.h"
#include <stx/random.h>
#include <brokerd/FeedService.h>

namespace stx {
namespace feeds {

class BrokerServlet : public stx::http::HTTPService {
public:

  BrokerServlet(FeedService* service);

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void getHostID(
      http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void insertRecord(
      http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void fetchRecords(
      http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  FeedService* service_;
};

}
}
#endif
