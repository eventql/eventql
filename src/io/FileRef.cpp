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
