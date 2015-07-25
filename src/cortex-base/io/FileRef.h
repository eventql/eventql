// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/Buffer.h>
#include <cstdint>
#include <unistd.h>

namespace cortex {

/**
 * Basic abstraction of an open file handle, its range, and auto-close feature.
 *
 * This FileRef represents an open file that is to be read starting
 * from the given @c offset up to @c size bytes.
 *
 * If the FileRef was initialized with auto-close set to on, its
 * underlying resource file descriptor will be automatically closed.
 */
class CORTEX_API FileRef {
 private:
  FileRef(const FileRef&) = delete;
  FileRef& operator=(const FileRef&) = delete;

 public:
  /** General move semantics for FileRef(FileRef&&). */
  FileRef(FileRef&& ref)
      : fd_(ref.fd_),
        offset_(ref.offset_),
        size_(ref.size_),
        close_(ref.close_) {
    ref.fd_ = -1;
    ref.close_ = false;
  }

  /** General move semantics for operator=. */
  FileRef& operator=(FileRef&& ref) {
    fd_ = ref.fd_;
    offset_ = ref.offset_;
    size_ = ref.size_;
    close_ = ref.close_;

    ref.fd_ = -1;
    ref.close_ = false;

    return *this;
  }

  /**
   * Initializes given FileRef.
   *
   * @param fd Underlying resource file descriptor.
   * @param offset The offset to start reading from.
   * @param size Number of bytes to read.
   * @param close Whether or not to close the underlying file desriptor upon
   *              object destruction.
   */
  FileRef(int fd, off_t offset, size_t size, bool close)
      : fd_(fd), offset_(offset), size_(size), close_(close) {}

  /**
   * Conditionally closes the underlying resource file descriptor.
   */
  ~FileRef() {
    if (close_) {
      ::close(fd_);
    }
  }

  int handle() const CORTEX_NOEXCEPT { return fd_; }

  off_t offset() const CORTEX_NOEXCEPT { return offset_; }
  void setOffset(off_t n) { offset_ = n; }

  bool empty() const noexcept { return size_ == 0; }
  size_t size() const CORTEX_NOEXCEPT { return size_; }
  void setSize(size_t n) { size_ = n; }

  void fill(Buffer* output) const;

 private:
  int fd_;
  off_t offset_;
  size_t size_;
  bool close_;
};

} // namespace cortex
