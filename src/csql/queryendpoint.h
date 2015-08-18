/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_METRICDB_HTTPINTERFACE_H
#define _FNORDMETRIC_METRICDB_HTTPINTERFACE_H
#include <memory>
#include <stx/uri.h>
#include <stx-http/httpservice.h>
#include <stx-http/httprequest.h>
#include <stx-http/httpresponse.h>
}
#include <stx/json/jsonoutputstream.h>

using namespace stx;
namespace csql {
class IMetric;
class IMetricRepository;

class QueryEndpoint : public http::HTTPService {
public:

  QueryEndpoint(stx::metric_service::IMetricRepository* metric_repo);

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response) override;

protected:

  stx::metric_service::IMetricRepository* metric_repo_;
};

}
#endif
