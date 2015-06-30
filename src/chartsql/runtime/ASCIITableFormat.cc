/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/ASCIITableFormat.h>

namespace csql {

void ASCIITableFormat::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context,
    ScopedPtr<OutputStream> output) {

  for (int i = 0; i < query->numStatements(); ++i) {
    output->write("==== query ====\n");

    query->executeStatement(
        context,
        i,
        [&output] (int argc, const csql::SValue* argv) -> bool {
      Vector<String> row;
      for (int n = 0; n < argc; ++n) {
        row.emplace_back(argv[n].toString());
      }

      output->write(fnord::inspect(row));
      output->write("\n");
      return true;
    });
  }
}

}
