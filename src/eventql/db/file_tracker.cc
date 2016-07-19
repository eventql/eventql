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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/inspect.h>
#include <eventql/util/random.h>
#include <eventql/util/io/fileutil.h>
#include "eventql/eventql.h"
#include "eventql/db/file_tracker.h"

namespace eventql {

FileTracker::FileTracker(const String& trash_dir) : trash_dir_(trash_dir) {}

void FileTracker::incrementRefcount(const String& filename) {
  std::unique_lock<std::mutex> lk(mutex_);
  ++refs_[filename];
}

void FileTracker::decrementRefcount(const String& filename) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (--refs_[filename] == 0) {
    refs_.erase(filename);
  }
}

bool FileTracker::isReferenced(const String& filename) const {
  std::unique_lock<std::mutex> lk(mutex_);
  const auto& iter = refs_.lower_bound(filename);
  if (iter == refs_.end() || !StringUtil::beginsWith(iter->first, filename)) {
    return false;
  } else {
    return true;
  }
}

size_t FileTracker::getNumReferencedFiles() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return refs_.size();
}

void FileTracker::deleteFiles(const Set<String>& filenames) {
  std::unique_lock<std::mutex> lk(mutex_);
  deleted_.insert(filenames.begin(), filenames.end());
  lk.unlock();

  auto trash_link = File::openFile(
      FileUtil::joinPaths(trash_dir_, Random::singleton()->hex128() + ".trash"),
      File::O_WRITE | File::O_CREATE);

  for (const auto& filename : filenames) {
    trash_link.write("//" + filename + "\n");
  }
}

void FileTracker::deleteFile(const String& filename) {
  std::unique_lock<std::mutex> lk(mutex_);
  deleted_.insert(filename);
  lk.unlock();

  auto trash_link = File::openFile(
      FileUtil::joinPaths(trash_dir_, Random::singleton()->hex128() + ".trash"),
      File::O_WRITE | File::O_CREATE);

  trash_link.write("//" + filename + "\n");
}

} // namespace eventql

