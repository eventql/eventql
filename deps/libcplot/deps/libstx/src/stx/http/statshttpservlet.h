/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_STATS_STATSHTTPSERVLET_H
#define _STX_STATS_STATSHTTPSERVLET_H
#include "stx/autoref.h"
#include "stx/http/httpservice.h"
#include "stx/stats/statsrepository.h"

namespace stx {
namespace stats {

class StatsHTTPServlet : public stx::http::HTTPService {
public:

  StatsHTTPServlet();
  StatsHTTPServlet(StatsRepository* stats_repo);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res);

protected:
  StatsRepository* stats_repo_;
};

}
}
#endif
