/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/random.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>

namespace zbase {

template <typename T>
SSTableSink<T>::SSTableSink(
    const String& tempdir) :
    output_file_(
        FileUtil::joinPaths(
            tempdir,
            "tmp." + Random::singleton()->hex64() + ".sst")) {}

template <typename T>
void SSTableSink<T>::open() {
  //stx::logInfo(
  //    "cm.reportbuild",
  //    "Opening output sstable: $0",
  //    output_file_);

  sstable_writer_ = sstable::SSTableEditor::create(
      output_file_,
      sstable::IndexProvider{},
      nullptr,
      0);
}

template <typename T>
void SSTableSink<T>::addRow(const String& key, const T& stats) {
  util::BinaryMessageWriter buf;
  stats.encode(&buf);
  sstable_writer_->appendRow(key.data(), key.size(), buf.data(), buf.size());
}

template <typename T>
RefPtr<VFSFile> SSTableSink<T>::finalize() {
  //stx::logInfo(
  //    "cm.reportbuild",
  //    "Finalizing output sstable: $0",
  //    output_file_);

  sstable_writer_->finalize();

  return new io::MmappedFile(
      File::openFile(output_file_, File::O_READ | File::O_AUTODELETE));
}

} // namespace zbase

