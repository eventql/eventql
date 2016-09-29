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
#ifndef _STX_IO_FILELOCK_H_
#define _STX_IO_FILELOCK_H_

#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>

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

#endif
