/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/sql/codec/ascii_codec.h>
#include <eventql/sql/runtime/resultlist.h>

namespace zbase {

ASCIICodec::ASCIICodec(csql::QueryPlan* query)  {
  for (size_t i = 0; i < query->numStatements(); ++i) {
    auto result = mkScoped(new csql::ResultList());
    query->storeResults(i, result.get());
    results_.emplace_back(std::move(result));
  }
}

void ASCIICodec::printResults(ScopedPtr<OutputStream> output) {
  for (size_t i = 0; i < results_.size(); ++i) {
    results_[i]->debugPrint(output.get());
  }
}

}
