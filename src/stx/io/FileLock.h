/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_IO_FILELOCK_H_
#define _STX_IO_FILELOCK_H_

#include <stx/stdtypes.h>
#include <stx/io/file.h>

namespace stx {

/**
 * A RAII File Lock class. Destructor automatically yields the lock if is was
 * aquired.
 */
class FileLock {
public:

  /**
   * Create a new lockfile at the specified path, but do not lock it yet
   */
  FileLock(const String& filename);

  FileLock() = delete;
  FileLock(const FileLock& copy) = delete;
  FileLock& operator=(const FileLock& copy) = delete;
  ~FileLock();

  /**
   * Lock the file. If the lock is busy, the behaviour depends on the block flag.
   *
   * If the block flag is set to true, the method will wait until the lock is
   * obtained (which might never happen).
   *
   * If the block flag is set to false the method will either return immediately
   * if the lock was aquired successfully or throw an exception if the lock is
   * busy.
   */
  void lock(bool block = false);

  /**
   * Release the lock. It is an error to call this method without calling lock
   * first.
   *
   * N.B. that the lock is also automatically unlocked once the FileLock dtor
   * is called
   */
  void unlock();

protected:
  String filename_;
  File file_;
  bool locked_;
};

}
#endif
