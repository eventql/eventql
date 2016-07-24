/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <string.h>
#include <eventql/util/exception.h>
#include <eventql/util/fnv.h>
#include <eventql/util/io/BufferedOutputStream.h>
#include <eventql/io/sstable/binaryformat.h>
#include <eventql/io/sstable/fileheaderwriter.h>
#include <eventql/io/sstable/fileheaderreader.h>
#include <eventql/io/sstable/SSTableWriter.h>
#include <eventql/io/sstable/SSTableColumnWriter.h>
#include <eventql/io/sstable/RowWriter.h>


namespace sstable {

std::unique_ptr<SSTableWriter> SSTableWriter::create(
    const std::string& filename,
    void const* header,
    size_t header_size) {
  auto file = File::openFile(
      filename,
      File::O_READ | File::O_WRITE | File::O_CREATE);

  MetaPage hdr(header, header_size);

  FileOutputStream os(file.fd());
  FileHeaderWriter::writeHeader(
      hdr,
      header,
      header_size,
      &os);

  return mkScoped(new SSTableWriter(std::move(file), hdr));
}

std::unique_ptr<SSTableWriter> SSTableWriter::reopen(
    const std::string& filename) {
  auto file = File::openFile(filename, File::O_READ | File::O_WRITE);

  FileInputStream is(file.fd());
  auto header = FileHeaderReader::readMetaPage(&is);

  return mkScoped(new SSTableWriter(std::move(file), header));
}

SSTableWriter::SSTableWriter(
    File&& file,
    MetaPage hdr) :
    file_(std::move(file)),
    hdr_(hdr),
    meta_dirty_(false) {}

SSTableWriter::~SSTableWriter() {
  commit();
}

uint64_t SSTableWriter::appendRow(
    void const* key,
    size_t key_size,
    void const* data,
    size_t data_size) {
  if (hdr_.isFinalized()) {
    RAISE(kIllegalStateError, "can't append row to finalized sstable");
  }

  if (data_size == 0) {
    RAISE(kIllegalArgumentError, "can't append empty row");
  }

  file_.seekTo(hdr_.bodyOffset() + hdr_.bodySize());
  BufferedOutputStream os(FileOutputStream::fromFileDescriptor(file_.fd()));
  auto rsize = RowWriter::appendRow(hdr_, key, key_size, data, data_size, &os);

  auto roff = hdr_.bodySize();
  hdr_.setBodySize(roff + rsize);
  hdr_.setRowCount(hdr_.rowCount() + 1);
  meta_dirty_ = true;

  return roff;
}

uint64_t SSTableWriter::appendRow(
    const std::string& key,
    const std::string& value) {
  return appendRow(key.data(), key.size(), value.data(), value.size());
}

uint64_t SSTableWriter::appendRow(
    const std::string& key,
    const SSTableColumnWriter& value) {
  return appendRow(key.data(), key.size(), value.data(), value.size());
}

void SSTableWriter::commit() {
  if (!meta_dirty_) {
    return;
  }

  file_.seekTo(0);
  FileOutputStream os(file_.fd());
  FileHeaderWriter::writeMetaPage(hdr_, &os);
  meta_dirty_ = false;
}

}
