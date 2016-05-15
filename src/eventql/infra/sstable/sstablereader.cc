/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/util/fnv.h>
#include <eventql/util/exception.h>
#include <eventql/util/inspect.h>
#include <eventql/util/io/VFSFileInputStream.h>
#include <eventql/infra/sstable/binaryformat.h>
#include <eventql/infra/sstable/binaryformat.h>
#include <eventql/infra/sstable/sstablereader.h>


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
      is_,
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
    RefPtr<RewindableInputStream> is,
    size_t begin,
    size_t limit) :
    is_(is),
    begin_(begin),
    limit_(limit),
    pos_(0),
    valid_(false),
    have_key_(false),
    have_value_(false) {
  seekTo(0);
}

bool SSTableReader::SSTableReaderCursor::fetchMeta() {
  BinaryFormat::RowHeader hdr;

  have_key_ = false;
  have_value_ = false;
  valid_ = false;

  if (begin_ + pos_ + sizeof(hdr) < limit_) {
    is_->readNextBytes(&hdr, sizeof(hdr));
    key_size_ = hdr.key_size;
    value_size_ = hdr.data_size;
    valid_ = true;
  }

  return valid_;
}

void SSTableReader::SSTableReaderCursor::seekTo(size_t body_offset) {
  pos_ = body_offset;
  is_->seekTo(begin_ + body_offset);
  fetchMeta();
}

bool SSTableReader::SSTableReaderCursor::trySeekTo(size_t body_offset) {
  if (begin_ + body_offset < limit_) {
    seekTo(body_offset);
    return true;
  } else {
    return false;
  }
}

size_t SSTableReader::SSTableReaderCursor::position() const {
  return pos_;
}

size_t SSTableReader::SSTableReaderCursor::nextPosition() {
  if (!valid_) {
    RAISE(kIllegalStateError, "invalid cursor");
  }

  return pos_ + sizeof(BinaryFormat::RowHeader) + key_size_ + value_size_;
}

bool SSTableReader::SSTableReaderCursor::next() {
  if (!have_key_) {
    is_->skipNextBytes(key_size_);
  }

  if (!have_value_) {
    is_->skipNextBytes(value_size_);
  }

  pos_ = nextPosition();
  return fetchMeta();
}

bool SSTableReader::SSTableReaderCursor::valid() {
  return valid_;
}

void SSTableReader::SSTableReaderCursor::getKey(void** data, size_t* size) {
  if (!valid_) {
    RAISE(kIllegalStateError, "invalid cursor");
  }

  if (!have_key_) {
    key_.resize(key_size_);
    is_->readNextBytes(key_.data(), key_size_);
    have_key_ = true;
  }

  *data = key_.data();
  *size = key_size_;
}

void SSTableReader::SSTableReaderCursor::getData(void** data, size_t* size) {
  if (!valid_) {
    RAISE(kIllegalStateError, "invalid cursor");
  }

  if (!have_key_) {
    key_.resize(key_size_);
    is_->readNextBytes(key_.data(), key_size_);
    have_key_ = true;
  }

  if (!have_value_) {
    value_.resize(value_size_);
    is_->readNextBytes(value_.data(), value_size_);
    have_value_ = true;
  }

  *data = value_.data();
  *size = value_size_;
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

