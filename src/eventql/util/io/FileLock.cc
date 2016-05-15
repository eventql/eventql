/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <sys/file.h>
#include <eventql/util/exception.h>
#include <eventql/util/io/FileLock.h>

FileLock::FileLock(
    const String& filename) :
    filename_(filename),
    file_(File::openFile(filename, File::O_CREATEOROPEN | File::O_WRITE)),
    locked_(false) {}

FileLock::~FileLock() {
  if (locked_) {
    unlock();
  }
}

void FileLock::lock(bool block /* = false */) {
  if (locked_) {
    RAISEF(kRuntimeError, "File is already locked: $0", filename_);
  }

  auto flag = LOCK_EX;
  if (!block) {
    flag |= LOCK_NB;
  }

  if (flock(file_.fd(), flag) != 0) {
    RAISE_ERRNO(kRuntimeError, "can't lock file: %s", filename_.c_str());
  }

  locked_ = true;
}

void FileLock::unlock() {
  if (!locked_) {
    RAISEF(kRuntimeError, "File is not locked: $0", filename_);
  }

  if (flock(file_.fd(), LOCK_UN) != 0) {
    RAISE_ERRNO(kRuntimeError, "can't unlock file: %s", filename_.c_str());
  }

  locked_ = false;
}
