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
#include <sstable/binaryformat.h>
#include <sstable/fileheaderwriter.h>
#include <sstable/fileheaderreader.h>
#include <sstable/SSTableWriter.h>
#include <sstable/SSTableColumnWriter.h>

namespace stx {
namespace sstable {

std::unique_ptr<SSTableWriter> SSTableWriter::create(
    const std::string& filename,
    void const* header,
    size_t header_size) {
  auto file = File::openFile(
      filename,
      File::O_READ | File::O_WRITE | File::O_CREATE);

  auto header_page = FileHeaderWriter::buildHeader(header, header_size);
  file.write(header_page.data(), header_page.size());

  // FIXPAUL!
  MemoryInputStream is(header_page.data(), header_page.size());
  auto hdr = FileHeaderReader::readMetaPage(&is);

  return mkScoped(new SSTableWriter(std::move(file), hdr));
}

std::unique_ptr<SSTableWriter> SSTableWriter::reopen(
    const std::string& filename) {
  auto file = File::openFile(filename, File::O_READ);

  FileInputStream is(file.clone()); // FIXPAUL
  auto header = FileHeaderReader::readMetaPage(&is);

  return mkScoped(new SSTableWriter(std::move(file), header));
}

SSTableWriter::SSTableWriter(
    File&& file,
    FileHeader hdr) :
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

  RAISE(kNotYetImplementedError);
  //size_t page_size = sizeof(BinaryFormat::RowHeader) + key_size + data_size;
  //auto alloc = mmap_->allocPage(page_size);
  //auto page = mmap_->getPage(alloc);

  //auto header = page->structAt<BinaryFormat::RowHeader>(0);
  //header->key_size = key_size;
  //header->data_size = data_size;

  //auto key_dst = page->structAt<void>(sizeof(BinaryFormat::RowHeader));
  //memcpy(key_dst, key, key_size);

  //auto data_dst = page->structAt<void>(
  //    sizeof(BinaryFormat::RowHeader) + key_size);
  //memcpy(data_dst, data, data_size);

  //FNV<uint32_t> fnv;
  //header->checksum = fnv.hash(
  //    page->structAt<void>(sizeof(uint32_t)),
  //    page_size - sizeof(uint32_t));

  //page->sync();

  //auto row_body_offset = body_size_;
  //body_size_ += page_size;

  //for (const auto& idx : indexes_) {
  //  idx->addRow(row_body_offset, key, key_size, data, data_size);
  //}

  //return row_body_offset;
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
}

//void SSTableWriter::writeIndex(uint32_t index_type, const Buffer& buf) {
//  writeIndex(index_type, buf.data(), buf.size());
//}
//
//void SSTableWriter::writeIndex(uint32_t index_type, void* data, size_t size) {
//  if (finalized_) {
//    RAISE(kIllegalStateError, "table is immutable (alread finalized)");
//  }
//
//  if (size == 0) {
//    return;
//  }
//
//  auto alloc = mmap_->allocPage(sizeof(BinaryFormat::FooterHeader) + size);
//  auto page = mmap_->getPage(alloc, io::MmapPageManager::kNoPadding{});
//
//  auto header = page->structAt<BinaryFormat::FooterHeader>(0);
//  header->magic = BinaryFormat::kMagicBytes;
//  header->type = index_type;
//  header->footer_size = size;
//
//  FNV<uint32_t> fnv;
//  header->footer_checksum = fnv.hash(data, size);
//
//  if (size > 0) {
//    auto dst = page->structAt<void>(sizeof(BinaryFormat::FooterHeader));
//    memcpy(dst, data, size);
//  }
//
//  page->sync();
//  mmap_->shrinkFile();
//}
//
//void SSTableWriter::reopen(size_t file_size) {
//}

// FIXPAUL lock
//void SSTableWriter::finalize() {
//  finalized_ = true;
//
//  auto page = mmap_->getPage(
//      io::PageManager::Page(0, FileHeaderWriter::calculateSize(0)));
//
//  FileHeaderWriter header(page->ptr(), page->size());
//  header.updateBodySize(body_size_);
//  header.setFlag(FileHeaderFlags::FINALIZED);
//
//  page->sync();
//  mmap_->shrinkFile();
//}

}
}
