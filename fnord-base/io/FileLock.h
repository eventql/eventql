/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_IO_FILELOCK_H_
#define _FNORD_IO_FILELOCK_H_

#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>

namespace fnord {

class FileLock {
public:

  FileLock(const String& filename);
  FileLock() = delete;
  FileLock(const FileLock& copy) = delete;
  FileLock& operator=(const FileLock& copy) = delete;
  ~FileLock();

  void lock(bool block = false);
  void unlock();

protected:
  String filename_;
  File file_;
  bool locked_;
};

}
#endif
