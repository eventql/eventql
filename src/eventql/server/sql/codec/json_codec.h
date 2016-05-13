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
#include <eventql/sql/result_cursor.h>

namespace zbase {

class JSONCodec {
public:

  JSONCodec(json::JSONOutputStream* json);

  void printResultTable(
      const Vector<String>& header,
      csql::ResultCursor* cursor);

protected:
  json::JSONOutputStream* json_;
};

}
