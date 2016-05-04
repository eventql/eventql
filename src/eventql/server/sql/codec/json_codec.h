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
#include <eventql/util/json/json.h>
#include <eventql/sql/runtime/queryplan.h>
#include <eventql/sql/runtime/charts/ChartStatement.h>
#include <eventql/sql/runtime/ResultFormat.h>

namespace zbase {

class JSONCodec {
public:

  static void formatResultList(
      const csql::ResultList* results,
      json::JSONOutputStream* json);

  JSONCodec(csql::QueryPlan* query);

  void printResults(ScopedPtr<OutputStream> output);

protected:
  Vector<ScopedPtr<csql::ResultList>> results_;
};

}
