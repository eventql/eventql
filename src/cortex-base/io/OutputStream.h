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
#include <cstdint>

namespace cortex {

class CORTEX_API OutputStream {
 public:
  virtual OutputStream() {}

  virtual void write(const char* buf, size_t size) = 0;
  virtual void write(const std::string& data);
  virtual void printf(const char* fmt, ...);
};

class CORTEX_API FileOutputStream : public OutputStream {
 public:
  FileOutputStream(int fd);
  ~FileOutputStream();

  int handle() const CORTEX_NOEXCEPT { return fd_; }

 private:
  int fd_;
};

class CORTEX_API BufferOutputStream : public OutputStream {
 public:
  BufferOutputStream(Buffer* sink);

 private:
  Buffer* sink_;
};

} // namespace cortex


















