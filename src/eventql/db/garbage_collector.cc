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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/logging.h>
#include <eventql/util/application.h>
#include <eventql/util/io/fileutil.h>
#include "eventql/eventql.h"
#include "eventql/db/garbage_collector.h"
#include "eventql/server/server_stats.h"
#include <algorithm>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace eventql {

String garbageCollectorModeToString(GarbageCollectorMode mode) {
  switch (mode) {
    case GarbageCollectorMode::DISABLED: return "DISABLED";
    case GarbageCollectorMode::MANUAL: return "MANUAL";
    case GarbageCollectorMode::AUTOMATIC: return "AUTOMATIC";
  }
}

GarbageCollectorMode garbageCollectorModeFromString(String str) {
  StringUtil::toUpper(&str);

  if (str == "DISABLED") {
    return GarbageCollectorMode::DISABLED;
  }

  if (str == "MANUAL") {
    return GarbageCollectorMode::MANUAL;
  }

  if (str == "AUTOMATIC") {
    return GarbageCollectorMode::AUTOMATIC;
  }

  RAISEF(kRuntimeError, "invalid garbage collector mode: $0", str);
}


GarbageCollector::GarbageCollector(
    GarbageCollectorMode mode,
    FileTracker* file_tracker,
    const String& data_dir,
    const String& trash_dir,
    const String& cache_dir,
    uint64_t cache_dir_maxsize /* = kDefaultCachedirMaxSize */,
    size_t gc_interval /* = kDefaultGCInterval */) :
    mode_(mode),
    file_tracker_(file_tracker),
    data_dir_(data_dir),
    trash_dir_(trash_dir),
    cache_dir_(cache_dir),
    cache_dir_maxsize_(cache_dir_maxsize),
    gc_interval_(kDefaultGCInterval) {}

GarbageCollector::~GarbageCollector() {
  assert(thread_running_ == false);
}

void GarbageCollector::runGC() {
  logDebug("evqld", "Running garbage collector...");

  emptyTrash();
  flushCache();
}

void GarbageCollector::startGCThread() {
  if (mode_ != GarbageCollectorMode::AUTOMATIC) {
    return;
  }

  thread_running_ = true;

  thread_ = std::thread([this] {
    Application::setCurrentThreadName("evqld-gc");

    std::unique_lock<std::mutex> lk(mutex_);
    while (thread_running_) {
      lk.unlock();

      try {
        runGC();
      } catch (const std::exception& e) {
        logError("evqld", e, "Error in GarbgageCollection thread");
      }

      lk.lock();
      cv_.wait_for(
          lk,
          std::chrono::microseconds(gc_interval_),
          [this] () { return !thread_running_; });
    }
  });
}

void GarbageCollector::stopGCThread() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (!thread_running_) {
    return;
  }

  thread_running_ = false;
  cv_.notify_all();
  lk.unlock();
  try {
    thread_.join();
  } catch (const std::exception& e) {
    /* ignore */
  }
}

void GarbageCollector::emptyTrash() {
  Set<String> trash_links;
  FileUtil::ls(trash_dir_, [&trash_links] (const String& filename) -> bool {
    if (StringUtil::endsWith(filename, ".trash")) {
      trash_links.insert(filename);
    }

    return true;
  });

  for (const auto& trash_link : trash_links) {
    auto trash_link_full = FileUtil::joinPaths(trash_dir_, trash_link);
    auto trash_link_contents = FileUtil::read(trash_link_full);
    auto filenames = StringUtil::split(trash_link_contents.toString(), "\n");
    bool trash_link_deleted = true;

    for (auto filename : filenames) {
      if (filename.empty()) {
        continue;
      }

      if (StringUtil::beginsWith(filename, "//")) {
        filename = filename.substr(2);
      } else if (StringUtil::beginsWith(filename, data_dir_)) {
        filename = filename.substr(data_dir_.size());
      } else {
        logWarning("evqld", "Invalid trash link: $0", filename);
        continue;
      }

      if (file_tracker_->isReferenced(filename)) {
        trash_link_deleted = false;
        break;
      }

      auto filename_full = FileUtil::joinPaths(data_dir_, filename);
      if (!FileUtil::exists(filename_full)) {
        continue;
      }

      if (mode_ == GarbageCollectorMode::DISABLED) {
        logDebug("evqld", "GC disabled, not deleting file: $0", filename_full);
      } else {
        logDebug("evqld", "Deleting file: $0", filename_full);
        FileUtil::rm(filename_full);
      }
    }

    if (!trash_link_deleted) {
      continue;
    }

    if (mode_ == GarbageCollectorMode::DISABLED) {
      logDebug("evqld", "GC disabled, not deleting file: $0", trash_link_full);
    } else {
      logDebug("evqld", "Deleting file: $0", trash_link_full);
      FileUtil::rm(trash_link_full);
    }
  }
}

struct CacheFile {
  String filename;
  uint64_t time;
  uint64_t size;
};

void GarbageCollector::flushCache() {
  Vector<CacheFile> cache_files;

  FileUtil::ls(cache_dir_, [this, &cache_files] (const String& f) -> bool {
    auto fpath = FileUtil::joinPaths(cache_dir_, f);

    struct stat fstat;
    if (stat(fpath.c_str(), &fstat) < 0) {
      logError(
          "evqld",
          "Error in GarbageCollector::flushCache: fstat('$0') failed",
          fpath.c_str());

      return true;
    }

    CacheFile cf;
    cf.filename = f;
    cf.size = fstat.st_size;
    cf.time = fstat.st_ctime;
    cache_files.emplace_back(cf);
    return true;
  });

  auto comparator = [] (const CacheFile& a, const CacheFile& b) -> bool {
    return b.time < a.time;
  };

  std::sort(cache_files.begin(), cache_files.end(), comparator);

  size_t cache_dir_size = 0;
  for (const auto& cf : cache_files) {
    if (cache_dir_size > cache_dir_maxsize_) {
      auto cache_filepath = FileUtil::joinPaths(cache_dir_, cf.filename);

      if (mode_ == GarbageCollectorMode::DISABLED) {
        logDebug("evqld", "GC disabled, not deleting file: $0", cache_filepath);
      } else {
        logDebug("evqld", "Deleting file: $0", cache_filepath);
        FileUtil::rm(cache_filepath);
      }
    } else {
      cache_dir_size += cf.size;
    }
  }

  evqld_stats()->cache_size.set(cache_dir_size);
}

} // namespace eventql

