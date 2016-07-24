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
#include <eventql/sql/runtime/query_cache.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/io/BufferedOutputStream.h>
#include <eventql/util/random.h>

namespace csql {

QueryCache::QueryCache(
    const String& cache_dir,
    size_t assoc_cache_size /* = kDefaultAssocCacheSize */,
    size_t cache_store_minhits /* = kDefaultCacheStoreMinHits */) :
    cache_dir_(cache_dir),
    assoc_cache_(assoc_cache_size),
    cache_store_minhits_(cache_store_minhits) {}

void QueryCache::getEntry(
    const SHA1Hash& key,
    Function<void (InputStream* is)> fn) {
  incrementHitcount(key);

  auto file_path = FileUtil::joinPaths(cache_dir_, key.toString() + ".qc");
  int file_fd = open(file_path.c_str(), O_RDONLY);
  if (file_fd < 0) {
    return;
  }

  auto file_is = FileInputStream::fromFileDescriptor(
      file_fd,
      true);

  fn(file_is.get());
}

void QueryCache::storeEntry(
    const SHA1Hash& key,
    Function<void (OutputStream* is)> fn) {
  if (getHitcount(key) < cache_store_minhits_) {
    return;
  }

  auto file_path = FileUtil::joinPaths(cache_dir_, key.toString() + ".qc");
  auto file_path_tmp = file_path + "." + Random::singleton()->hex64();
  auto file_os = BufferedOutputStream::fromStream(
      FileOutputStream::openFile(file_path_tmp));

  fn(file_os.get());
  file_os->flush();
  // ignore errors
  ::rename(file_path_tmp.c_str(), file_path.c_str());
}

void QueryCache::incrementHitcount(const SHA1Hash& key) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto key_finger = *static_cast<const uint64_t*>(key.data());
  auto& slot = assoc_cache_[key_finger % assoc_cache_.size()];
  if (slot.key == key) {
    ++slot.hitcount;
  } else {
    slot.key = key;
    slot.hitcount = 1;
  }
}

size_t QueryCache::getHitcount(const SHA1Hash& key) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto key_finger = *static_cast<const uint64_t*>(key.data());
  const auto& slot = assoc_cache_[key_finger % assoc_cache_.size()];
  if (slot.key == key) {
    return slot.hitcount;
  } else {
    return 0;
  }
}

} // namespace csql
