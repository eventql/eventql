/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/JoinedQueryTableSource.h"
#include "fnord-json/json.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

using namespace fnord;

namespace cm {

JoinedQueryTableSource::JoinedQueryTableSource(
    const String& sstable_filename) :
    JoinedQueryTableSource(Set<String> { sstable_filename }) {}

JoinedQueryTableSource::JoinedQueryTableSource(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {}

void JoinedQueryTableSource::read() {
  auto rep_start_time = std::numeric_limits<uint64_t>::max();
  auto rep_end_time = std::numeric_limits<uint64_t>::min();

  for (const auto& sstable : input_files_) {
    fnord::logInfo("cm.reportbuild", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    if (!reader.isFinalized()) {
      RAISEF(kRuntimeError, "unfinished sstable: $0", sstable);
    }

    if (reader.bodySize() == 0) {
      fnord::logWarning("cm.reportbuild", "Warning: empty sstable: $0", sstable);
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
      auto val = cursor->getDataBuffer();
      Option<cm::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<cm::JoinedQuery>(val));
      } catch (const Exception& e) {
        fnord::logWarning("cm.reportbuild", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        for (const auto& cb : callbacks_) {
          cb(q.get());
        }
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

void JoinedQueryTableSource::forEach(CallbackFn fn) {
  callbacks_.emplace_back(fn);
}

Set<String> JoinedQueryTableSource::inputFiles() {
  return input_files_;
}

} // namespace cm

