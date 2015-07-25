/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <sys/file.h>
#include <stx/exception.h>
#include <stx/io/FileLock.h>

namespace stx {

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

}
