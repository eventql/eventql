// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/FlowLocation.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace xzero {
namespace flow {

std::string FlowLocation::str() const {
  char buf[256];
  std::size_t n =
      snprintf(buf, sizeof(buf), "{ %zu:%zu.%zu - %zu:%zu.%zu }", begin.line,
               begin.column, begin.offset, end.line, end.column, end.offset);
  return std::string(buf, n);
}

std::string FlowLocation::text() const {
  std::string result;
  char* buf = nullptr;
  ssize_t size;
  ssize_t n;
  int fd;

  fd = open(filename.c_str(), O_RDONLY);
  if (fd < 0) return std::string();

  size = 1 + end.offset - begin.offset;
  if (size <= 0) goto out;

  if (lseek(fd, begin.offset, SEEK_SET) < 0) goto out;

  buf = new char[size + 1];
  n = read(fd, buf, size);
  if (n < 0) goto out;

  result = std::string(buf, n);

out:
  delete[] buf;
  close(fd);
  return result;
}

}  // namespace flow
}  // namespace xzero
