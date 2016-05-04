/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/json/json.h>
#include <stx/io/fileutil.h>
#include <stx/random.h>
#include "eventql/TermInfoTableSink.h"

using namespace stx;

namespace zbase {

TermInfoTableSink::TermInfoTableSink(
    const String& tempdir) :
    output_file_(
        FileUtil::joinPaths(
            tempdir,
            "tmp." + Random::singleton()->hex64() + ".sst")) {
  sstable_schema_.addColumn(
      "related_terms",
      1,
      sstable::SSTableColumnType::STRING);

  sstable_schema_.addColumn(
      "top_categories",
      2,
      sstable::SSTableColumnType::STRING);

  sstable_schema_.addColumn("score", 3, sstable::SSTableColumnType::FLOAT);
}

void TermInfoTableSink::open() {
  sstable_writer_ = sstable::SSTableEditor::create(
      output_file_,
      sstable::IndexProvider{},
      nullptr,
      0);
}

void TermInfoTableSink::addRow(const String& key, const TermInfo& term_info) {
  String terms_str;
  for (const auto t : term_info.related_terms) {
    terms_str += StringUtil::format("$0:$1,", t.first, t.second);
  }

  String topcats_str;
  for (const auto t : term_info.top_categories) {
    topcats_str += StringUtil::format("$0:$1,", t.first, t.second);
  }

  sstable::SSTableColumnWriter cols(&sstable_schema_);
  cols.addStringColumn(1, terms_str);
  cols.addStringColumn(2, topcats_str);
  cols.addFloatColumn(3, term_info.score);
  sstable_writer_->appendRow(key, cols);
}

RefPtr<VFSFile> TermInfoTableSink::finalize() {
  sstable_schema_.writeIndex(sstable_writer_.get());
  sstable_writer_->finalize();

  return new io::MmappedFile(
      File::openFile(output_file_, File::O_READ | File::O_AUTODELETE));
}

} // namespace zbase

