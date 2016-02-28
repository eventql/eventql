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
#include <stx/stdtypes.h>
#include <stx/http/HTTPSSEStream.h>

namespace zbase {

class JSONSSECodec{
public:

  JSONSSECodec(
      csql::QueryPlan* query,
      RefPtr<http::HTTPSSEStream> sse_stream);

protected:

  void sendResult(size_t idx);
  void sendProgress(double progress);

  RefPtr<http::HTTPSSEStream> output_;
  Vector<ScopedPtr<csql::ResultList>> results_;
};

}
