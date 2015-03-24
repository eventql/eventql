
/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-json/json.h>
#include "reports/CTRCounterSSTableSink.h"

using namespace fnord;

namespace cm {

CTRCounterSSTableSink::CTRCounterSSTableSink(
    const String& output_file) :
    output_file_(output_file) {
  sstable_schema_.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
}

void CTRCounterSSTableSink::onEvent(
    ReportEventType type,
    ReportEventTime time,
    void* ev) {
  switch (type) {

    case ReportEventType::BEGIN: {
      if (FileUtil::exists(output_file_ + "~")) {
        fnord::logInfo(
            "cm.ctrstats",
            "Deleting orphaned temp file: $0",
            output_file_ + "~");

        FileUtil::rm(output_file_ + "~");
      }

      fnord::logInfo(
          "cm.ctrstats",
          "Opening output sstable: $0",
          output_file_);

      HashMap<String, String> out_hdr;
      out_hdr["start_time"] =
          StringUtil::toString(time.get().first.unixMicros());
      out_hdr["end_time"] =
          StringUtil::toString(time.get().second.unixMicros());
      auto outhdr_json = json::toJSONString(out_hdr);

      sstable_writer_ = sstable::SSTableWriter::create(
          output_file_ + "~",
          sstable::IndexProvider{},
          outhdr_json.data(),
          outhdr_json.length());

      return;
    }

    case ReportEventType::CTR_COUNTER: {
      sstable::SSTableColumnWriter cols(&sstable_schema_);
      cols.addUInt64Column(1, ((CTRCounter*) ev)->second.num_views);
      cols.addUInt64Column(2, ((CTRCounter*) ev)->second.num_clicks);
      cols.addUInt64Column(3, ((CTRCounter*) ev)->second.num_clicked);
      sstable_writer_->appendRow(((CTRCounter*) ev)->first, cols);
      return;
    }

    case ReportEventType::END: {
      fnord::logInfo(
          "cm.ctrstats",
          "Finalizing output sstable: $0",
          output_file_);

      sstable_schema_.writeIndex(sstable_writer_.get());
      sstable_writer_->finalize();

      FileUtil::mv(output_file_ + "~", output_file_);
      return;
    }

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

Set<String> CTRCounterSSTableSink::outputFiles() {
  return Set<String> { output_file_ };
}

} // namespace cm

