/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stx/buffer.h>
#include <stx/exception.h>
#include <stx/io/inputstream.h>
#include <stx/ieee754.h>

namespace stx {

std::unique_ptr<InputStream> InputStream::getStdin() {
  auto stdin_stream = new FileInputStream(0, false);
  return std::unique_ptr<InputStream>(stdin_stream);
}

InputStream::InputStream(
    const std::string& filename /* = "<anonymous input stream>" */) :
    filename_(filename) {}

const std::string& InputStream::getFileName() const {
  return filename_;
}

void InputStream::setFileName(const std::string& filename) {
  filename_ = filename;
}

// FIXPAUL: optimize?
size_t InputStream::readNextBytes(std::string* target, size_t n_bytes) {
  char byte;
  size_t length;

  for (length = 0; length < n_bytes && readNextByte(&byte); ++length) {
    *target += byte;
  }

  return length;
}

// FIXPAUL: optimize?
size_t InputStream::readNextBytes(Buffer* target, size_t n_bytes) {
  char byte;
  size_t length;

  target->reserve(n_bytes);

  for (length = 0; length < n_bytes && readNextByte(&byte); ++length) {
    target->append(&byte, sizeof(byte));
  }

  return length;
}

size_t InputStream::readNextBytes(void* target, size_t n_bytes) {
  size_t length = 0;

  while (length < n_bytes && readNextByte(((char*) target) + length)) {
    ++length;
  }

  return length;
}

// FIXPAUL: optimize?
size_t InputStream::readUntilEOF(std::string* target) {
  char byte;
  size_t length;

  for (length = 0; readNextByte(&byte); ++length) {
    *target += byte;
  }

  return length;
}

bool InputStream::readLine(std::string* target) {
  char byte;
  size_t length;

  for (length = 0; readNextByte(&byte); ++length) {
    *target += byte;

    if (byte == '\n') {
      return true;
    }
  }

  return length > 0;
}

uint8_t InputStream::readUInt8() {
  uint8_t val;
  if (readNextBytes(&val, sizeof(uint8_t)) != sizeof(uint8_t)) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

uint16_t InputStream::readUInt16() {
  uint16_t val;
  if (readNextBytes(&val, sizeof(uint16_t)) != sizeof(uint16_t)) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

uint32_t InputStream::readUInt32() {
  uint32_t val;
  if (readNextBytes(&val, sizeof(uint32_t)) != sizeof(uint32_t)) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

uint64_t InputStream::readUInt64() {
  uint64_t val;
  if (readNextBytes(&val, sizeof(uint64_t)) != sizeof(uint64_t)) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

uint64_t InputStream::readVarUInt() {
  uint64_t val = 0;

  for (int i = 0; ; ++i) {
    unsigned char b;
    if (!readNextByte((char*) &b)) {
      RAISE(kEOFError, "unexpected end of stream");
    }

    val |= (b & 0x7fULL) << (7 * i);

    if (!(b & 0x80U)) {
      break;
    }
  }

  return val;
}

String InputStream::readString(size_t size) {
  String val;
  if (readNextBytes(&val, size) != size) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

String InputStream::readLenencString() {
  auto size = readVarUInt();

  String val;
  if (readNextBytes(&val, size) != size) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return val;
}

double InputStream::readDouble() {
  uint64_t val;
  if (readNextBytes(&val, sizeof(uint64_t)) != sizeof(uint64_t)) {
    RAISE(kEOFError, "unexpected end of stream");
  }

  return IEEE754::fromBytes(val);
}


std::unique_ptr<FileInputStream> FileInputStream::openFile(
    const std::string& file_path) {
  auto fp = file_path.c_str();
  int fd = open(fp, O_RDONLY);

  if (fd < 1) {
    RAISE_ERRNO(kIOError, "error opening file '%s'", fp);
  }

  std::unique_ptr<FileInputStream> stream(new FileInputStream(fd, true));
  stream->readNextChunk();
  stream->setFileName(file_path);
  return stream;
}

std::unique_ptr<FileInputStream> FileInputStream::fromFileDescriptor(
    int fd,
    bool close_on_destroy /* = false */) {
  std::unique_ptr<FileInputStream> stream(
      new FileInputStream(fd, close_on_destroy));

  stream->setFileName(StringUtil::format("<fd:$0>", fd));
  return stream;
}

std::unique_ptr<FileInputStream> FileInputStream::fromFile(File&& file) {
  auto fd = file.fd();

  std::unique_ptr<FileInputStream> stream(new FileInputStream(std::move(file)));
  stream->setFileName(StringUtil::format("<fd:$0>", fd));
  return stream;
}

FileInputStream::FileInputStream(
    int fd,
    bool close_on_destroy /* = false */) :
    fd_(fd),
    close_on_destroy_(close_on_destroy),
    buf_len_(0),
    buf_pos_(0) {}

FileInputStream::FileInputStream(
    File&& file) :
    FileInputStream(file.releaseFD(), true) {}

FileInputStream::~FileInputStream() {
  if (fd_ >= 0 && close_on_destroy_) {
    close(fd_);
  }
}

bool FileInputStream::readNextByte(char* target) {
  if (buf_pos_ >= buf_len_) {
    readNextChunk();
  }

  if (buf_pos_ < buf_len_) {
    *target = buf_[buf_pos_++];
    return true;
  } else {
    return false;
  }
}

size_t FileInputStream::skipNextBytes(size_t nbytes) {
  auto buf_remaining = buf_len_ - buf_pos_;

  if (nbytes <= buf_remaining) {
    buf_pos_ += nbytes;
    return nbytes;
  }

  buf_pos_ = 0;
  buf_len_ = 0;

  if (lseek(fd_, nbytes - buf_remaining, SEEK_CUR) == -1) {
    RAISE_ERRNO("seek() failed");
  }

  return nbytes;
}

bool FileInputStream::eof() {
  if (buf_pos_ >= buf_len_) {
    readNextChunk();
  }

  return buf_pos_ >= buf_len_;
}

// FIXPAUL move somwhere else...
FileInputStream::kByteOrderMark FileInputStream::readByteOrderMark() {
  if (buf_pos_ >= buf_len_) {
    readNextChunk();
  }

  static char kByteOrderMarkUTF8[] = { '\xEF', '\xBB', '\xBF' };
  if (buf_pos_ + sizeof(kByteOrderMarkUTF8) <= buf_len_ &&
      strncmp(
          buf_ + buf_pos_,
          kByteOrderMarkUTF8,
          sizeof(kByteOrderMarkUTF8)) == 0) {
    buf_pos_ += sizeof(kByteOrderMarkUTF8);
    return BOM_UTF8;
  }

  static char kByteOrderMarkUTF16[] = { '\xFF', '\xFE' };
  if (buf_pos_ + sizeof(kByteOrderMarkUTF16) <= buf_len_ &&
      strncmp(
          buf_ + buf_pos_,
          kByteOrderMarkUTF16,
          sizeof(kByteOrderMarkUTF16)) == 0) {
    buf_pos_ += sizeof(kByteOrderMarkUTF16);
    return BOM_UTF16;
  }

  return BOM_UNKNOWN;
}

void FileInputStream::readNextChunk() {
  int bytes_read = read(fd_, buf_, sizeof(buf_));

  if (bytes_read < 0) {
    RAISE_ERRNO(kIOError, "read(%s) failed", getFileName().c_str());
  }

  buf_pos_ = 0;
  buf_len_ = bytes_read;
}

void FileInputStream::rewind() {
  buf_pos_ = 0;
  buf_len_ = 0;

  if (lseek(fd_, 0, SEEK_SET) < 0) {
    RAISE_ERRNO(kIOError, "lseek(%s) failed", getFileName().c_str());
  }
}

std::unique_ptr<StringInputStream> StringInputStream::fromString(
    const std::string& string) {
  return std::unique_ptr<StringInputStream>(new StringInputStream(string));
}

StringInputStream::StringInputStream(
    const std::string& string) :
    str_(string),
    cur_(0) {
}

bool StringInputStream::readNextByte(char* target) {
  if (cur_ < str_.size()) {
    *target = str_[cur_++];
    return true;
  } else {
    return false;
  }
}

size_t StringInputStream::skipNextBytes(size_t n_bytes) {
  auto nskipped = n_bytes;
  if (cur_ + n_bytes > str_.size()) {
    nskipped = str_.size() - cur_;
  }

  cur_ += nskipped;
  return nskipped;
}

bool StringInputStream::eof() {
  return cur_ >= str_.size();
}

void StringInputStream::rewind() {
  cur_ = 0;
}

std::unique_ptr<BufferInputStream> BufferInputStream::fromBuffer(
    const Buffer* buf) {
  return std::unique_ptr<BufferInputStream>(new BufferInputStream(buf));
}

BufferInputStream::BufferInputStream(
    const Buffer* buf) :
    buf_(buf),
    cur_(0) {
}

bool BufferInputStream::readNextByte(char* target) {
  if (cur_ < buf_->size()) {
    *target = buf_->charAt(cur_++);
    return true;
  } else {
    return false;
  }
}

size_t BufferInputStream::skipNextBytes(size_t n_bytes) {
  auto nskipped = n_bytes;
  if (cur_ + n_bytes > buf_->size()) {
    nskipped = buf_->size() - cur_;
  }

  cur_ += nskipped;
  return nskipped;
}

bool BufferInputStream::eof() {
  return cur_ >= buf_->size();
}

void BufferInputStream::rewind() {
  cur_ = 0;
}

MemoryInputStream::MemoryInputStream(
    const void* data,
    size_t size) :
    data_(data),
    size_(size),
    cur_(0) {}

bool MemoryInputStream::readNextByte(char* target) {
  if (cur_ < size_) {
    *target = *(((char*) data_) + cur_++);
    return true;
  } else {
    return false;
  }
}

size_t MemoryInputStream::skipNextBytes(size_t n_bytes) {
  auto nskipped = n_bytes;
  if (cur_ + n_bytes > size_) {
    nskipped = size_ - cur_;
  }

  cur_ += nskipped;
  return nskipped;
}

bool MemoryInputStream::eof() {
  return cur_ >= size_;
}

void MemoryInputStream::rewind() {
  cur_ = 0;
}

} // namespace stx

