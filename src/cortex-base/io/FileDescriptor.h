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

namespace cortex {

/**
 * Represents a system file descriptor that gets automatically closed.
 */
class CORTEX_API FileDescriptor {
 public:
  FileDescriptor(int fd) : fd_(fd) {}
  FileDescriptor(FileDescriptor&& fd) : fd_(fd.release()) {}
  ~FileDescriptor() { close(); }

  FileDescriptor& operator=(FileDescriptor&& fd);

  FileDescriptor(const FileDescriptor& fd) = delete;
  FileDescriptor& operator=(const FileDescriptor& fd) = delete;

  int get() const noexcept { return fd_; }
  operator int() const noexcept { return fd_; }

  int release();
  void close();

 private:
  int fd_;
};

}  // namespace cortex
