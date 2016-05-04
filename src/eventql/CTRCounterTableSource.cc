/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/CTRCounterTableSource.h"
#include <eventql/infra/sstable/SSTableColumnReader.h>
#include "stx/json/json.h"

using namespace stx;

namespace zbase {

CTRCounterTableSource::CTRCounterTableSource(
    const List<dproc::TaskDependency>& deps) :
    deps_(deps) {
  sstable_schema_.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
}

void CTRCounterTableSource::read(dproc::TaskContext* context) {
  Pair<UnixTime, UnixTime> rep_time(0, 0);

  int row_idx = 0;
  for (size_t i = 0; i < deps_.size(); ++i) {
    /* read sstable header */
    sstable::SSTableReader reader(context->getDependency(i)->getData());

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      CTRCounterData c;
      auto key = cursor->getKeyString();

      auto cols_data = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&sstable_schema_, cols_data);
      c.num_views = cols.getUInt64Column(
          sstable_schema_.columnID("num_views"));
      c.num_clicks = cols.getUInt64Column(
          sstable_schema_.columnID("num_clicks"));
      c.num_clicked = cols.getUInt64Column(
          sstable_schema_.columnID("num_clicked"));

      for (const auto& cb : callbacks_) {
        cb(key, c);
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

void CTRCounterTableSource::forEach(CallbackFn fn) {
  callbacks_.emplace_back(fn);
}

List<dproc::TaskDependency> CTRCounterTableSource::dependencies() const {
  return deps_;
}

} // namespace zbase

