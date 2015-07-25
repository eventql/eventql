// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/MemoryFile.h>
#include <cortex-base/io/MemoryMap.h>
#include <cortex-base/io/FileDescriptor.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/hash/FNV.h>
#include <cortex-base/io/FileUtil.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/sysconfig.h>

#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace cortex {

// Since OS/X doesn't support SHM in a way Linux does, we need to work around it.
#if CORTEX_OS_DARWIN
#  define CORTEX_MEMORYFILE_USE_TMPFILE
#else
// Use TMPFILE here, too (for now), because create*Channel would fail otherwise.
// So let's create a posix_filebuf
//#  define CORTEX_MEMORYFILE_USE_SHM
#  define CORTEX_MEMORYFILE_USE_TMPFILE
#endif

MemoryFile::MemoryFile()
    : File("", ""),
      mtime_(0),
      inode_(0),
      etag_(),
      fspath_() {
}

MemoryFile::MemoryFile(
    const std::string& path,
    const std::string& mimetype,
    const BufferRef& data,
    DateTime mtime)
    : File(path, mimetype),
      mtime_(mtime.unixtime()),
      inode_(0),
      size_(data.size()),
      etag_(std::to_string(data.hash())),
      fspath_(),
      fd_(-1) {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  fspath_ = FileUtil::joinPaths(FileUtil::tempDirectory(), "memfile.XXXXXXXX");

  fd_ = mkstemp(const_cast<char*>(fspath_.c_str()));
  if (fd_ < 0)
    RAISE_ERRNO(errno);

  if (ftruncate(fd_, size()) < 0)
    RAISE_ERRNO(errno);

  ssize_t n = pwrite(fd_, data.data(), data.size(), 0);
  if (n < 0)
    RAISE_ERRNO(errno);
#else
  // TODO: URL-escape instead
  fspath_ = path;
  StringUtil::replaceAll(&fspath_, "/", "\%2f");

  if (fspath_.size() >= NAME_MAX)
    RAISE_ERRNO(ENAMETOOLONG);

  FileDescriptor fd = shm_open(fspath_.c_str(), O_RDWR | O_CREAT, 0600);
  if (fd < 0)
    RAISE_ERRNO(errno);

  if (ftruncate(fd, size()) < 0)
    RAISE_ERRNO(errno);

  ssize_t n = pwrite(fd, data.data(), data.size(), 0);
  if (n < 0)
    RAISE_ERRNO(errno);
#endif

  if (static_cast<size_t>(n) != data.size())
    RAISE_ERRNO(EIO);
}

MemoryFile::~MemoryFile() {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  if (fd_ >= 0) {
    ::close(fd_);
  }
#else
  shm_unlink(fspath_.c_str());
#endif
}

const std::string& MemoryFile::etag() const {
  return etag_;
}

size_t MemoryFile::size() const CORTEX_NOEXCEPT {
  return size_;
}

time_t MemoryFile::mtime() const CORTEX_NOEXCEPT {
  return mtime_;
}

size_t MemoryFile::inode() const CORTEX_NOEXCEPT {
  return inode_;
}

bool MemoryFile::isRegular() const CORTEX_NOEXCEPT {
  return true;
}

bool MemoryFile::isDirectory() const CORTEX_NOEXCEPT {
  return false;
}

bool MemoryFile::isExecutable() const CORTEX_NOEXCEPT {
  return false;
}

int MemoryFile::createPosixChannel(OpenFlags oflags) {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  if (fd_ < 0) {
    errno = ENOENT;
    return -1;
  }

  // XXX when using dup(fd_) we'd also need to fcntl() the flags.
  // - Both having advantages / disadvantages.
  return ::open(fspath_.c_str(), to_posix(oflags));
#else
  return shm_open(fspath_.c_str(), to_posix(oflags), 0600);
#endif
}

std::unique_ptr<std::istream> MemoryFile::createInputChannel() {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  return std::unique_ptr<std::istream>(
      new std::ifstream(fspath_, std::ios::binary));
#else
  RAISE(RuntimeError, "Not implemented.");
#endif
}

std::unique_ptr<std::ostream> MemoryFile::createOutputChannel() {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  return std::unique_ptr<std::ostream>(
      new std::ofstream(fspath_, std::ios::binary));
#else
  RAISE(RuntimeError, "Not implemented.");
#endif
}

std::unique_ptr<MemoryMap> MemoryFile::createMemoryMap(bool rw) {
#if defined(CORTEX_MEMORYFILE_USE_TMPFILE)
  // use FileDescriptor for auto-closing here, in case of exceptions
  FileDescriptor fd = ::open(fspath_.c_str(), rw ? O_RDWR : O_RDONLY);
  if (fd < 0)
    RAISE_ERRNO(errno);

  return std::unique_ptr<MemoryMap>(new MemoryMap(fd, 0, size(), rw));
#else
  return std::unique_ptr<MemoryMap>(new MemoryMap(fd_, 0, size(), rw));
#endif
}

} // namespace cortex
