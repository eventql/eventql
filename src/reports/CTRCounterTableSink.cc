
/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-json/json.h>
#include "reports/CTRCounterTableSink.h"

using namespace fnord;

namespace cm {

CTRCounterTableSink::CTRCounterTableSink(
    DateTime start_time,
    DateTime end_time,
    const String& output_file) :
    start_time_(start_time),
    end_time_(end_time),
    output_file_(output_file) {
  sstable_schema_.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
}

void CTRCounterTableSink::open() {
  if (FileUtil::exists(output_file_ + "~")) {
    fnord::logInfo(
        "cm.reportbuild",
        "Deleting orphaned temp file: $0",
        output_file_ + "~");

    FileUtil::rm(output_file_ + "~");
  }

  fnord::logInfo(
      "cm.reportbuild",
      "Opening output sstable: $0",
      output_file_);

  HashMap<String, String> out_hdr;
  out_hdr["start_time"] =
      StringUtil::toString(start_time_.unixMicros());
  out_hdr["end_time"] =
      StringUtil::toString(end_time_.unixMicros());
  auto outhdr_json = json::toJSONString(out_hdr);

  sstable_writer_ = sstable::SSTableWriter::create(
      output_file_ + "~",
      sstable::IndexProvider{},
      outhdr_json.data(),
      outhdr_json.length());
}

void CTRCounterTableSink::addRow(const String& key, CTRCounterData counter) {
  sstable::SSTableColumnWriter cols(&sstable_schema_);
  cols.addUInt64Column(1, counter.num_views);
  cols.addUInt64Column(2, counter.num_clicks);
  cols.addUInt64Column(3, counter.num_clicked);
  sstable_writer_->appendRow(key, cols);
}

void CTRCounterTableSink::close() {
  fnord::logInfo(
      "cm.reportbuild",
      "Finalizing output sstable: $0",
      output_file_);

  sstable_schema_.writeIndex(sstable_writer_.get());
  sstable_writer_->finalize();

  FileUtil::mv(output_file_ + "~", output_file_);
}

Set<String> CTRCounterTableSink::outputFiles() {
  return Set<String> { output_file_ };
}

} // namespace cm

