/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/json/json.h>
#include <stx/random.h>
#include <stx/io/fileutil.h>
#include "zbase/CTRCounterTableSink.h"

using namespace stx;

namespace cm {

CTRCounterTableSink::CTRCounterTableSink(
    const String& tempdir) :
    output_file_(FileUtil::joinPaths(tempdir, Random::singleton()->hex64())) {
  sstable_schema_.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema_.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
}

void CTRCounterTableSink::open() {
  sstable_writer_ = sstable::SSTableEditor::create(
      output_file_,
      sstable::IndexProvider{},
      nullptr,
      0);
}

void CTRCounterTableSink::addRow(const String& key, CTRCounterData counter) {
  sstable::SSTableColumnWriter cols(&sstable_schema_);
  cols.addUInt64Column(1, counter.num_views);
  cols.addUInt64Column(2, counter.num_clicks);
  cols.addUInt64Column(3, counter.num_clicked);
  sstable_writer_->appendRow(key, cols);
}

RefPtr<VFSFile> CTRCounterTableSink::finalize() {
  sstable_schema_.writeIndex(sstable_writer_.get());
  sstable_writer_->finalize();

  return new io::MmappedFile(
      File::openFile(output_file_, File::O_READ | File::O_AUTODELETE));
}



} // namespace cm

