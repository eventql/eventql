/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/http/HTTPSSEStream.h>
#include <eventql/server/sql/codec/json_codec.h>

namespace zbase {

class JSONSSECodec{
public:

  JSONSSECodec(
      csql::QueryPlan* query,
      RefPtr<http::HTTPSSEStream> sse_stream);

protected:

  void sendResults();
  void sendProgress(double progress);

  JSONCodec json_codec_;
  RefPtr<http::HTTPSSEStream> output_;
};

}
