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
#include <functional>
#include <string>
#include <stdint.h>

namespace cortex {

class Buffer;
class File;

class CORTEX_API FileUtil {
 public:
  static char pathSeperator() noexcept;

  static std::string currentWorkingDirectory();

  static std::string realpath(const std::string& relpath);
  static bool exists(const std::string& path);
  static bool isDirectory(const std::string& path);
  static bool isRegular(const std::string& path);
  static size_t size(const std::string& path);
  static size_t sizeRecursive(const std::string& path);
  CORTEX_DEPRECATED static size_t du_c(const std::string& path) { return sizeRecursive(path); }
  static void ls(const std::string& path, std::function<bool(const std::string&)> cb);

  static std::string joinPaths(const std::string& base, const std::string& append);

  static Buffer read(int fd);
  static Buffer read(File& file);
  static Buffer read(const std::string& path);
  static void write(const std::string& path, const Buffer& buffer);
  static void copy(const std::string& from, const std::string& to);
  static void truncate(const std::string& path, size_t size);
  static void mkdir(const std::string& path, int mode = 0775);
  static void mkdir_p(const std::string& path, int mode = 0775);
  static void rm(const std::string& path);
  static void mv(const std::string& path, const std::string& target);

  static int createTempFile();
  static int createTempFileAt(const std::string& basedir,
                              std::string* result = nullptr);
  static std::string createTempDirectory();
  static std::string tempDirectory();

  static void allocate(int fd, size_t length);
  static void preallocate(int fd, off_t offset, size_t length);
  static void deallocate(int fd, off_t offset, size_t length);

  /**
   * Collapses given range of a file.
   *
   * @note Operating systems that do not support it natively will be emulated.
   */
  static void collapse(int fd, off_t offset, size_t length);

  static void truncate(int fd, size_t length);
};

}  // namespace cortex
