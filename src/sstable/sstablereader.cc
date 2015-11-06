/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <stx/exception.h>
#include <stx/inspect.h>
#include <stx/io/VFSFileInputStream.h>
#include <sstable/binaryformat.h>
#include <sstable/binaryformat.h>
#include <sstable/sstablereader.h>

namespace stx {
namespace sstable {


SSTableReader::SSTableReader(
    const String& filename) :
    SSTableReader(FileInputStream::openFile(filename)) {}

SSTableReader::SSTableReader(
    File&& file) :
    SSTableReader(FileInputStream::fromFile(std::move(file))) {}

SSTableReader::SSTableReader(
    RefPtr<VFSFile> vfs_file) :
    SSTableReader(new VFSFileInputStream(vfs_file)) {}

SSTableReader::SSTableReader(
    RefPtr<RewindableInputStream> is) :
    is_(is),
    header_(FileHeaderReader::readMetaPage(is_.get())) {
  //if (!header_.verify()) {
  //  RAISE(kIllegalStateError, "corrupt sstable header");
  //}

  //if (header_.headerSize() + header_.bodySize() > file_size_) {
  //  RAISE(kIllegalStateError, "file metadata offsets exceed file bounds");
  //}
}

Buffer SSTableReader::readHeader() {
  Buffer buf(header_.userdataSize());
  is_->seekTo(header_.userdataOffset());
  is_->readNextBytes(buf.data(), buf.size());
  return buf;
}

Buffer SSTableReader::readFooter(uint32_t type) {
  is_->seekTo(header_.headerSize() + header_.bodySize());

  while (!is_->eof()) {
    BinaryFormat::FooterHeader footer_header;
    is_->readNextBytes(&footer_header, sizeof(footer_header));

    if (footer_header.magic != BinaryFormat::kMagicBytes) {
      RAISE(kIllegalStateError, "corrupt sstable footer");
    }

    if (type && footer_header.type == type) {
      Buffer buf(footer_header.footer_size);
      is_->readNextBytes(buf.data(), buf.size());

      FNV<uint32_t> fnv;
      auto checksum = fnv.hash(buf.data(), buf.size());

      if (checksum != footer_header.footer_checksum) {
        RAISE(kIllegalStateError, "footer checksum mismatch. corrupt sstable?");
      }

      return buf;
    } else {
      is_->skipNextBytes(footer_header.footer_size);
    }
  }

  RAISE(kNotFoundError, "footer not found");
}

std::unique_ptr<SSTableReader::SSTableReaderCursor> SSTableReader::getCursor() {
  auto cursor = new SSTableReaderCursor(
      nullptr,
      header_.headerSize(),
      header_.headerSize() + header_.bodySize());

  return std::unique_ptr<SSTableReaderCursor>(cursor);
}

bool SSTableReader::isFinalized() const {
  return header_.isFinalized();
}

size_t SSTableReader::bodySize() const {
  return header_.bodySize();
}

size_t SSTableReader::bodyOffset() const {
  return header_.headerSize();
}

size_t SSTableReader::headerSize() const {
  return header_.userdataSize();
}

SSTableReader::SSTableReaderCursor::SSTableReaderCursor(
    RefPtr<VFSFile> mmap,
    size_t begin,
    size_t limit) :
    mmap_(std::move(mmap)),
    begin_(begin),
    limit_(limit),
    pos_(begin) {}

void SSTableReader::SSTableReaderCursor::seekTo(size_t body_offset) {
  pos_ = begin_ + body_offset;
}

bool SSTableReader::SSTableReaderCursor::trySeekTo(size_t body_offset) {
  if (begin_ + body_offset < limit_) {
    pos_ = begin_ + body_offset;
    return true;
  } else {
    return false;
  }
}

size_t SSTableReader::SSTableReaderCursor::position() const {
  return pos_ - begin_;
}

size_t SSTableReader::SSTableReaderCursor::nextPosition() {
  auto header = mmap_->structAt<BinaryFormat::RowHeader>(pos_);

  auto next_pos = pos_ + sizeof(BinaryFormat::RowHeader) +
      header->key_size +
      header->data_size;

  return next_pos - begin_;
}

bool SSTableReader::SSTableReaderCursor::next() {
  auto header = mmap_->structAt<BinaryFormat::RowHeader>(pos_);

  auto next_pos = pos_ += sizeof(BinaryFormat::RowHeader) +
      header->key_size +
      header->data_size;

  if (next_pos < limit_) {
    pos_ = next_pos;
  } else {
    return false;
  }

  return valid();
}

bool SSTableReader::SSTableReaderCursor::valid() {
  auto header = mmap_->structAt<BinaryFormat::RowHeader>(pos_);

  auto row_limit = pos_ + sizeof(BinaryFormat::RowHeader) +
      header->key_size +
      header->data_size;

  return row_limit <= limit_;
}

void SSTableReader::SSTableReaderCursor::getKey(void** data, size_t* size) {
  if (!valid()) {
    RAISE(kIllegalStateError, "invalid cursor");
  }

  auto header = mmap_->structAt<BinaryFormat::RowHeader>(pos_);
  *data = mmap_->structAt<void>(pos_ + sizeof(BinaryFormat::RowHeader));
  *size = header->key_size;
}

void SSTableReader::SSTableReaderCursor::getData(void** data, size_t* size) {
  if (!valid()) {
    RAISE(kIllegalStateError, "invalid cursor");
  }

  auto header = mmap_->structAt<BinaryFormat::RowHeader>(pos_);
  auto offset = pos_ + sizeof(BinaryFormat::RowHeader) + header->key_size;
  *data = mmap_->structAt<void>(offset);
  *size = header->data_size;
}

size_t SSTableReader::countRows() {
  size_t n = header_.rowCount();

  if (n == uint64_t(-1)) {
    auto cursor = getCursor();
    for (n = 0; cursor->valid(); ++n) {
      if (!cursor->next()) {
        break;
      }
    }
  }

  return n;
}

}
}

