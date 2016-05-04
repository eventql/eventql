/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_UTIL_FILEUTIL_H_
#define _libstx_UTIL_FILEUTIL_H_
#include "stx/buffer.h"
#include "stx/stdtypes.h"

namespace stx {

class FileUtil {
public:

  /**
   * Create a new directory
   */
  static void mkdir(const std::string& dirname);

  /**
   * Create one or more directories recursively
   */
  static void mkdir_p(const std::string& dirname);

  /**
   * Check if a file exists
   */
  static bool exists(const std::string& dirname);

  /**
   * Check if a file exists and is a directory
   */
  static bool isDirectory(const std::string& dirname);

  /**
   * Return the size of the file
   */
  static size_t size(const std::string& filename);

  /**
   * Return the last modification time of the file
   */
  static uint64_t mtime(const std::string& filename);

  /**
   * Return the inode number of the file
   */
  static uint64_t inodeID(const std::string& filename);

  /**
   * Return the last access time of the file
   */
  static uint64_t atime(const std::string& filename);

  /**
   * Join two paths
   */
  static std::string joinPaths(const std::string& p1, const std::string p2);

  /**
   * Base path
   */
  static std::string basePath(const std::string& path);

  /**
   * List files in a directory
   */
  static void ls(
      const std::string& dirname,
      std::function<bool(const std::string&)> callback);

  /**
   * Delete a file
   */
  static void rm(const std::string& filename);

  /**
   * Move a file
   */
  static void mv(const std::string& src, const std::string& dst);

  /**
   * Truncate a file
   */
  static void truncate(const std::string& filename, size_t size);

  /**
   * Read a file
   */
  static Buffer read(
      const std::string& filename,
      size_t offset = 0,
      size_t limit = 0);

  /**
   * Checsum (FNV64) a file
   */
  static uint64_t checksum(const std::string& filename);

  /**
   * Write a while file
   */
  static void write(const std::string& filename, const Buffer& data);

  /**
   * Copy a file
   */
  static void cp(const std::string& src, const std::string& destination);

  /**
   * Concat one file to another file
   */
  static void cat(const std::string& src, const std::string& target);

  /**
   * Return the size of a directory (like du -c)
   */
  static size_t du_c(const std::string& path);

};

}
#endif
