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
#include <cortex-base/Buffer.h>
#include <functional>
#include <iosfwd>
#include <string>
#include <memory>
#include <fcntl.h> // O_* flags for createPosixChannel()

namespace cortex {

class MemoryMap;

/**
 * HTTP servable file.
 *
 * @see FileHandler
 * @see LocalFile, MemoryFile
 * @see FileRepository
 */
class CORTEX_API File {
 public:
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  File(const std::string& path, const std::string& mimetype);

  virtual ~File();

  std::string filename() const;
  const std::string& path() const { return path_; }
  const std::string& mimetype() const { return mimetype_; }
  const std::string& lastModified() const;

  virtual const std::string& etag() const = 0;

  virtual size_t size() const CORTEX_NOEXCEPT = 0;
  virtual time_t mtime() const CORTEX_NOEXCEPT = 0;
  virtual size_t inode() const CORTEX_NOEXCEPT = 0;
  virtual bool isRegular() const CORTEX_NOEXCEPT = 0;
  virtual bool isDirectory() const CORTEX_NOEXCEPT = 0;
  virtual bool isExecutable() const CORTEX_NOEXCEPT = 0;

  /**
   * Flags that can be passed when creating a system file handle.
   *
   * @see createPosixChannel(OpenFlags oflags)
   */
  enum OpenFlags {
    Read        = 0x0001, // O_RDONLY
    Write       = 0x0002, // O_WRONLY
    ReadWrite   = 0x0003, // O_RDWR
    Create      = 0x0004, // O_CREAT
    CreateNew   = 0x0008, // O_EXCL
    Truncate    = 0x0010, // O_TRUNC
    Append      = 0x0020, // O_APPEND
    Share       = 0x0040, // O_CLOEXEC negagted
    NonBlocking = 0x0080, // O_NONBLOCK
    TempFile    = 0x0100, // O_TMPFILE
  };

  /**
   * Converts given OpenFlags to POSIX compatible flags.
   */
  static int to_posix(OpenFlags oflags);

  /** Creates a POSIX file handle with given flags.
   *
   * @param oflags such as O_RDONLY or O_NONBLOCK, etc (from <fcntl.h>)
   */
  virtual int createPosixChannel(OpenFlags oflags) = 0;

  /** Creates an input stream for given file. */
  virtual std::unique_ptr<std::istream> createInputChannel() = 0;

  /** Creates an output stream for given file. */
  virtual std::unique_ptr<std::ostream> createOutputChannel() = 0;

  /** Creates a memory-map for a given file.
   *
   * @param rw weather to map file in read/write mode, or just in read-only.
   */
  virtual std::unique_ptr<MemoryMap> createMemoryMap(bool rw = true) = 0;

  /**
   * Sets file-error code, that is only used for the validity of the entity.
   */
  void setErrorCode(int ec) { errno_ = ec; }

  /**
   * Retrieves errno-compatible error code for the validity of the entity.
   */
  int errorCode() const CORTEX_NOEXCEPT { return errno_; }

  /**
   * Tests wheather this file exists, that is, no error had occurred during
   * vfs_stat() operation.
   */
  bool exists() const noexcept { return errno_ == 0; }

 private:
  std::string path_;
  std::string mimetype_;

  int errno_;

 protected:
  mutable std::string lastModified_;
};

CORTEX_API File::OpenFlags operator|(File::OpenFlags a, File::OpenFlags b);

// {{{ inlines
CORTEX_API inline File::OpenFlags operator|(File::OpenFlags a,
                                           File::OpenFlags b) {
  return (File::OpenFlags) ((unsigned) a | (unsigned) b);
}
// }}}

} // namespace cortex
