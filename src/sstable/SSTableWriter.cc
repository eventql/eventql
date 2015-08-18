/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <stx/exception.h>
#include <stx/fnv.h>
#include <stx/io/BufferedOutputStream.h>
#include <sstable/binaryformat.h>
#include <sstable/fileheaderwriter.h>
#include <sstable/fileheaderreader.h>
#include <sstable/SSTableWriter.h>
#include <sstable/SSTableColumnWriter.h>
#include <sstable/RowWriter.h>

namespace stx {
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
}
