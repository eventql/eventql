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

ASCIITableFormat::ASCIITableFormat(
    ScopedPtr<OutputStream> output) :
    output_(std::move(output)) {}

void ASCIITableFormat::formatResults(
    RefPtr<QueryPlan> query,
    ExecutionContext* context) {

  for (int i = 0; i < query->numStatements(); ++i) {
    output_->write("==== query ====\n");

    auto stmt = query->buildStatement(i);

    stmt->execute(
        context,
        [this] (int argc, const csql::SValue* argv) -> bool {
      Vector<String> row;
      for (int n = 0; n < argc; ++n) {
        row.emplace_back(argv[n].toString());
      }

      output_->write(fnord::inspect(row));
      output_->write("\n");
      return true;
    });
  }
}

}
