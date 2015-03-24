/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRCounterSSTableSource.h"
#include "fnord-json/json.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

using namespace fnord;

namespace cm {

CTRCounterSSTableSource::CTRCounterSSTableSource(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {}

void CTRCounterSSTableSource::onEvent(ReportEventType type, void* ev) {
  switch (type) {
    case ReportEventType::BEGIN:
      emitEvent(type, ev);
      readTables();
      return;

    case ReportEventType::END:
      emitEvent(type, ev);
      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

void CTRCounterSSTableSource::readTables() {
  int row_idx = 0;
  int tbl_idx = 0;
  for (const auto& sstable : input_files_) {
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    if (reader.bodySize() == 0) {
      RAISEF(kRuntimeError, "unfinished sstable: $0", sstable);
    }

    /* read report header */
    //auto hdr = json::parseJSON(reader.readHeader());

    //auto tbl_start_time = json::JSONUtil::objectGetUInt64(
    //    hdr.begin(),
    //    hdr.end(),
    //    "start_time").get();

    //auto tbl_end_time = json::JSONUtil::objectGetUInt64(
    //    hdr.begin(),
    //    hdr.end(),
    //    "end_time").get();

    //if (tbl_start_time < start_time) {
    //  start_time = tbl_start_time;
    //}

    //if (tbl_end_time > end_time) {
    //  end_time = tbl_end_time;
    //}

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      auto val = cursor->getDataBuffer();
      Option<cm::JoinedQuery> q;

      //try {
      //  q = Some(json::fromJSON<cm::JoinedQuery>(val));
      //} catch (const Exception& e) {
      //  fnord::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      //}

      //if (!q.isEmpty()) {
      //  emitEvent(ReportEventType::JOINED_QUERY, &q.get());
      //}

      if (!cursor->next()) {
        break;
      }
    }
  }
}

Set<String> CTRCounterSSTableSource::inputFiles() {
  return input_files_;
}

} // namespace cm

