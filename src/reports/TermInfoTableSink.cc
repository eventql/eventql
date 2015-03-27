/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-json/json.h>
#include "reports/TermInfoTableSink.h"

using namespace fnord;

namespace cm {

TermInfoTableSink::TermInfoTableSink(
    const String& output_file) :
    output_file_(output_file) {
  sstable_schema_.addColumn("related_terms", 1, sstable::SSTableColumnType::STRING);
}

void TermInfoTableSink::open() {
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

  sstable_writer_ = sstable::SSTableWriter::create(
      output_file_ + "~",
      sstable::IndexProvider{},
      nullptr,
      0);
}

void TermInfoTableSink::addRow(
      const String& lang,
      const String& term,
      const HashMap<String, uint64_t>& related_terms) {
  auto key = lang + "~" + term;

  String terms_str;
  for (const auto t : related_terms) {
    terms_str += StringUtil::format("$0:$1,", t.first, t.second);
  }

  sstable::SSTableColumnWriter cols(&sstable_schema_);
  cols.addStringColumn(1, terms_str);
  sstable_writer_->appendRow(key, cols);
}

void TermInfoTableSink::close() {
  fnord::logInfo(
      "cm.reportbuild",
      "Finalizing output sstable: $0",
      output_file_);

  sstable_schema_.writeIndex(sstable_writer_.get());
  sstable_writer_->finalize();

  FileUtil::mv(output_file_ + "~", output_file_);
}

Set<String> TermInfoTableSink::outputFiles() {
  return Set<String> { output_file_ };
}

} // namespace cm

