// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/io/FileRef.h>
#include <xzero/sysconfig.h>
#include <system_error>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>

namespace xzero {

void FileRef::fill(Buffer* output) const {
  output->reserve(output->size() + size());

#if defined(HAVE_PREAD)
  ssize_t n = pread(handle(), output->end(), size(), offset());
  if (n < 0)
    throw std::system_error(errno, std::system_category());

  if (n != size())
    throw std::runtime_error("Did not read all required bytes from FileRef.");

  output->resize(n);
#else
# error "Implementation missing"
#endif
}

} // namespace xzero
