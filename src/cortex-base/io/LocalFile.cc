// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/LocalFile.h>
#include <cortex-base/io/LocalFileRepository.h>
#include <cortex-base/io/MemoryMap.h>
#include <cortex-base/io/FileDescriptor.h>
#include <cortex-base/MimeTypes.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/sysconfig.h>

#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace cortex {

LocalFile::LocalFile(LocalFileRepository& repo,
                     const std::string& path,
                     const std::string& mimetype)
    : File(path, mimetype),
      repo_(repo),
      stat_(),
      etag_() {
  update();
}

LocalFile::~LocalFile() {
}

size_t LocalFile::size() const CORTEX_NOEXCEPT {
  return stat_.st_size;
}

time_t LocalFile::mtime() const CORTEX_NOEXCEPT {
  return stat_.st_mtime;
}

size_t LocalFile::inode() const CORTEX_NOEXCEPT {
  return stat_.st_ino;
}

bool LocalFile::isRegular() const CORTEX_NOEXCEPT {
  return S_ISREG(stat_.st_mode);
}

bool LocalFile::isDirectory() const CORTEX_NOEXCEPT {
  return S_ISDIR(stat_.st_mode);
}

bool LocalFile::isExecutable() const CORTEX_NOEXCEPT {
  return stat_.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH);
}

const std::string& LocalFile::etag() const {
  // compute ETag response header value on-demand
  if (etag_.empty()) {
    size_t count = 0;
    Buffer buf(256);
    buf.push_back('"');

    if (repo_.etagConsiderMTime_) {
      if (count++) buf.push_back('-');
      buf.push_back(mtime());
    }

    if (repo_.etagConsiderSize_) {
      if (count++) buf.push_back('-');
      buf.push_back(size());
    }

    if (repo_.etagConsiderINode_) {
      if (count++) buf.push_back('-');
      buf.push_back(inode());
    }

    buf.push_back('"');

    etag_ = buf.str();
  }

  return etag_;
}

void LocalFile::update() {
  int rv = stat(path().c_str(), &stat_);

  if (rv < 0)
    setErrorCode(errno);
  else
    setErrorCode(0);
}

int LocalFile::createPosixChannel(OpenFlags oflags) {
  return ::open(path().c_str(), to_posix(oflags));
}

std::unique_ptr<std::istream> LocalFile::createInputChannel() {
  return std::unique_ptr<std::istream>(
      new std::ifstream(path(), std::ios::binary));
}

std::unique_ptr<std::ostream> LocalFile::createOutputChannel() {
  return std::unique_ptr<std::ostream>(
      new std::ofstream(path(), std::ios::binary));
}

std::unique_ptr<MemoryMap> LocalFile::createMemoryMap(bool rw) {
  // use FileDescriptor for auto-closing here, in case of exceptions
  FileDescriptor fd = ::open(path().c_str(), rw ? O_RDWR : O_RDONLY);
  if (fd < 0)
    RAISE_ERRNO(errno);

  return std::unique_ptr<MemoryMap>(new MemoryMap(fd, 0, size(), rw));
}

std::shared_ptr<LocalFile> LocalFile::get(const std::string& path) {
  static MimeTypes mimetypes;
  static LocalFileRepository repo(mimetypes, "/", true, true, false);
  return std::static_pointer_cast<LocalFile>(repo.getFile(path, "/"));
}

} // namespace cortex
