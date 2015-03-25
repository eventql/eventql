/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRCounterTableSource.h"
#include <fnord-sstable/SSTableColumnReader.h>
#include "fnord-json/json.h"

using namespace fnord;

namespace cm {

CTRCounterTableSource::CTRCounterTableSource(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {
  sstable_schema_.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
}

void CTRCounterTableSource::read() {
  auto rep_start_time = std::numeric_limits<uint64_t>::max();
  auto rep_end_time = std::numeric_limits<uint64_t>::min();

  for (const auto& sstable : input_files_) {
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    if (!reader.isFinalized()) {
      RAISEF(kRuntimeError, "unfinished sstable: $0", sstable);
    }

    if (reader.bodySize() == 0) {
      fnord::logWarning("cm.ctrstats", "Warning: empty sstable: $0", sstable);
    }

    /* read report header */
    auto hdr = json::parseJSON(reader.readHeader());

    auto tbl_start_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "start_time").get();

    auto tbl_end_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "end_time").get();

    if (tbl_start_time < rep_start_time) {
      rep_start_time = tbl_start_time;
    }

    if (tbl_end_time > rep_end_time) {
      rep_end_time = tbl_end_time;
    }
  }

  Pair<DateTime, DateTime> rep_time(rep_start_time, rep_end_time);

  int row_idx = 0;
  int tbl_idx = 0;
  for (const auto& sstable : input_files_) {
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

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

Set<String> CTRCounterTableSource::inputFiles() {
  return input_files_;
}

} // namespace cm

