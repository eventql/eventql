// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/FileRef.h>
#include <cortex-base/sysconfig.h>
#include <system_error>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>

namespace cortex {

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

} // namespace cortex
