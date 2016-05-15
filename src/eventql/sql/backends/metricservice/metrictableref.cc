/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/backends/metricservice/metrictableref.h>
#include <eventql/sql/tasks/tablescan.h>
#include <eventql/sql/svalue.h>

namespace csql {

MetricTableRef::MetricTableRef(
    stx::metric_service::IMetric* metric) :
    metric_(metric) {}

int MetricTableRef::getColumnIndex(const std::string& name) {
  if (name == "time") {
    return 0;
  }

  if (name == "value") {
    return 1;
  }

  if (metric_->hasLabel(name)) {
    fields_.emplace_back(name);
    return fields_.size() + 1;
  }

  return -1;
}

std::string MetricTableRef::getColumnName(int index) {
  if (index == 0) {
    return "time";
  }

  if (index == 1) {
    return "value";
  }

  if (index - 2 >= fields_.size()) {
    RAISE(kIndexError, "no such column");
  }

  return fields_[index - 2];
}

std::vector<std::string> MetricTableRef::columns() {
  auto columns = fields_;
  columns.emplace_back("value");
  columns.emplace_back("time");
  return columns;
}

void MetricTableRef::executeScan(csql::TableScan* scan) {
  auto begin = stx::UnixTime::epoch();
  auto limit = stx::UnixTime::now();

  metric_->scanSamples(
      begin,
      limit,
      [this, scan] (stx::metric_service::Sample* sample) -> bool {
        std::vector<csql::SValue> row;
        row.emplace_back(sample->time());
        row.emplace_back(sample->value());

        // FIXPAUL slow!
        for (const auto& field : fields_) {
          bool found = false;

          for (const auto& label : sample->labels()) {
            if (label.first == field) {
              found = true;
              row.emplace_back(label.second);
              break;
            }
          }

          if (!found) {
            row.emplace_back();
          }
        }

        return scan->nextRow(row.data(), row.size());
      });
}

}
