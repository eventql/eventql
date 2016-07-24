/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <dirent.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "eventql/util/buffer.h"
#include "eventql/util/fnv.h"
#include "eventql/util/exception.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/file.h"
#include "eventql/util/io/mmappedfile.h"

void FileUtil::mkdir(const std::string& dirname) {
  if (::mkdir(dirname.c_str(), S_IRWXU) != 0) {
    RAISE_ERRNO(kIOError, "mkdir('%s') failed", dirname.c_str());
  }
}

bool FileUtil::exists(const std::string& filename) {
  struct stat fstat;

  if (stat(filename.c_str(), &fstat) < 0) {
    if (errno == ENOENT) {
      return false;
    }

    RAISE_ERRNO(kIOError, "fstat('%s') failed", filename.c_str());
  }

  return true;
}

bool FileUtil::isDirectory(const std::string& filename) {
  struct stat fstat;

  if (stat(filename.c_str(), &fstat) < 0) {
    RAISE_ERRNO(kIOError, "fstat('%s') failed", filename.c_str());
  }

  return S_ISDIR(fstat.st_mode);
}

size_t FileUtil::size(const std::string& filename) {
  struct stat fstat;

  if (stat(filename.c_str(), &fstat) < 0) {
    RAISE_ERRNO(kIOError, "fstat('%s') failed", filename.c_str());
  }

  return fstat.st_size;
}

uint64_t FileUtil::inodeID(const std::string& path) {
  struct stat st;

  if (stat(path.c_str(), &st) < 0) {
    RAISE_ERRNO(kIOError, "fstat('%s') failed", path.c_str());
  }

  return (uint64_t) st.st_ino;
}

uint64_t FileUtil::mtime(const std::string& filename) {
  struct stat fstat;

  if (stat(filename.c_str(), &fstat) < 0) {
    RAISE_ERRNO(kIOError, "fstat('%s') failed", filename.c_str());
  }

  return fstat.st_mtime;
}

uint64_t FileUtil::atime(const std::string& filename) {
  struct stat fstat;

  if (stat(filename.c_str(), &fstat) < 0) {
    RAISE_ERRNO(kIOError, "fstat('%s') failed", filename.c_str());
  }

  return fstat.st_atime;
}

/* The mkdir_p method was adapted from bash 4.1 */
void FileUtil::mkdir_p(const std::string& dirname) {
  char const* begin = dirname.c_str();
  char const* cur = begin;

  if (exists(dirname)) {
    if (isDirectory(dirname)) {
      return;
    } else {
      RAISE(
          kIOError,
          "file '%s' exists but is not a directory",
          dirname.c_str());
    }
  }

  for (cur = begin; *cur == '/'; ++cur);

  while ((cur = strchr(cur, '/'))) {
    std::string path(begin, cur);
    cur++;

    if (exists(path)) {
      if (isDirectory(path)) {
        continue;
      } else {
        RAISE(
            kIOError,
            "file '%s' exists but is not a directory",
            path.c_str());
      }
    }

    mkdir(path);
  }

  mkdir(dirname);
}

std::string FileUtil::joinPaths(const std::string& p1, const std::string p2) {
  String p1_stripped = p1;
  StringUtil::stripTrailingSlashes(&p1_stripped);
  String p2_stripped = p2;
  StringUtil::stripTrailingSlashes(&p2_stripped);
  return p1_stripped + "/" + p2_stripped;
}

std::string FileUtil::basePath(const std::string& path) {
  String base_path = path;

  while (base_path.length() > 0 && base_path.back() != '/') {
    base_path.pop_back();
  }

  if (base_path.length() > 1) {
    base_path.pop_back();
  }

  return base_path;
}

void FileUtil::ls(
    const std::string& dirname,
    std::function<bool(const std::string&)> callback) {
  if (exists(dirname)) {
    if (!isDirectory(dirname)) {
      RAISE(
          kIOError,
          "file '%s' exists but is not a directory",
          dirname.c_str());
    }
  } else {
    RAISE(
        kIOError,
        "file '%s' does not exist",
        dirname.c_str());
  }

  auto dir = opendir(dirname.c_str());

  if (dir == nullptr) {
    RAISE_ERRNO("opendir(%s) failed", dirname.c_str());
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
#if defined(__APPLE__)
    size_t namlen = entry->d_namlen;
#else
    size_t namlen = strlen(entry->d_name);
#endif
    if (namlen < 1 || *entry->d_name == '.') {
      continue;
    }

    if (!callback(std::string(entry->d_name, namlen))) {
      break;
    }
  }

  closedir(dir);
}

void FileUtil::ls(
    const std::string& dirname,
    std::function<bool(const struct dirent*)> callback) {
  if (exists(dirname)) {
    if (!isDirectory(dirname)) {
      RAISE(
          kIOError,
          "file '%s' exists but is not a directory",
          dirname.c_str());
    }
  } else {
    RAISE(
        kIOError,
        "file '%s' does not exist",
        dirname.c_str());
  }

  auto dir = opendir(dirname.c_str());

  if (dir == nullptr) {
    RAISE_ERRNO("opendir(%s) failed", dirname.c_str());
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
#if defined(__APPLE__)
    size_t namlen = entry->d_namlen;
#else
    size_t namlen = strlen(entry->d_name);
#endif
    if (namlen < 1 || *entry->d_name == '.') {
      continue;
    }

    if (!callback(entry)) {
      break;
    }
  }

  closedir(dir);
}

void FileUtil::rm(const std::string& filename) {
  unlink(filename.c_str());
}

void FileUtil::mv(const std::string& src, const std::string& dst) {
  if (::rename(src.c_str(), dst.c_str()) < 0) {
    RAISE_ERRNO(kIOError, "rename(%s, %s) failed", src.c_str(), dst.c_str());
  }
}

void FileUtil::truncate(const std::string& filename, size_t new_size) {
  if (::truncate(filename.c_str(), new_size) < 0) {
    RAISE_ERRNO(kIOError, "truncate(%s) failed", filename.c_str());
  }
}

Buffer FileUtil::read(
    const std::string& filename,
    size_t offset /* = 0 */,
    size_t limit /* = 0 */) {
  try {
    auto file = File::openFile(filename, File::O_READ);
    if (offset > 0) {
      file.seekTo(offset);
    }

    Buffer buf(limit == 0 ? file.size() - offset : limit);
    auto read_bytes = file.read(&buf);
    buf.truncate(read_bytes);

    return buf;
  } catch (const std::exception& e) {
    RAISEF(kIOError, "$0 while reading file '$1'", e.what(), filename);
  }
}

uint64_t FileUtil::checksum(const std::string& filename) {
  MmappedFile mmap(File::openFile(filename, File::O_READ));
  FNV<uint64_t> fnv;
  return fnv.hash(mmap.data(), mmap.size());
}

void FileUtil::write(const std::string& filename, const Buffer& data) {
  auto file = File::openFile(
      filename,
      File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  file.write(data);
}

void FileUtil::cp(const std::string& src, const std::string& dst) {
  auto infile = File::openFile(src, File::O_READ);
  auto outfile = File::openFile(dst, File::O_WRITE | File::O_CREATE);

  Buffer buf(4096);
  size_t bytes;
  while ((bytes = infile.read(&buf)) > 0) {
    outfile.write(buf.data(), bytes);
  }
}

void FileUtil::cat(const std::string& src, const std::string& target) {
  auto infile = File::openFile(src, File::O_READ);
  auto outfile = File::openFile(target, File::O_WRITE | File::O_APPEND);

  Buffer buf(4096);
  size_t bytes;
  while ((bytes = infile.read(&buf)) > 0) {
    outfile.write(buf.data(), bytes);
  }
}

size_t FileUtil::du_c(const std::string& path) {
  size_t size = 0;

  FileUtil::ls(path, [&path, &size] (const String& file) -> bool {
    auto filename = FileUtil::joinPaths(path, file);

    if (FileUtil::isDirectory(filename)) {
      size += FileUtil::du_c(filename);
    } else {
      size += FileUtil::size(filename);
    }

    return true;
  });

  return size;
}


