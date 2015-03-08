/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-sstable/SSTableScan.h>

namespace fnord {
namespace sstable {

SSTableScan::SSTableScan(SSTableColumnSchema* schema) {}

void SSTableScan::execute(
    Cursor* cursor,
    Function<void (const Vector<String> row)> fn) {
  size_t num_rows = 0;

  for (; cursor->valid(); ++num_rows) {
    auto key = cursor->getKeyString();

    Vector<String> row;
    row.emplace_back(key);
    fn(row);
    //auto val = cursor->getDataBuffer();
    //sstable::SSTableColumnReader cols(&schema, val);
    //auto num_views = cols.getUInt64Column(schema.columnID("num_views"));

    if (!cursor->next()) {
      break;
    }
  }

}

} // namespace sstable
} // namespace fnord
