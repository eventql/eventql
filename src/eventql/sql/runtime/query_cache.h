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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/util/io/outputstream.h>

namespace csql {

class QueryCache {
public:
  static const size_t kDefaultAssocCacheSize = 8192;
  static const size_t kDefaultCacheStoreMinHits = 2;

  QueryCache(
      const String& cache_dir,
      size_t assoc_cache_size = kDefaultAssocCacheSize,
      size_t cache_store_minhits = kDefaultCacheStoreMinHits);

  void getEntry(
      const SHA1Hash& key,
      Function<void (InputStream* is)> fn);

  void storeEntry(
      const SHA1Hash& key,
      Function<void (OutputStream* is)> fn);

protected:

  void incrementHitcount(const SHA1Hash& key);
  size_t getHitcount(const SHA1Hash& key) const;

  struct CacheEntryHitcount {
    SHA1Hash key;
    size_t hitcount;
  };

  String cache_dir_;
  Vector<CacheEntryHitcount> assoc_cache_;
  size_t cache_store_minhits_;
  mutable std::mutex mutex_;
};

} // namespace csql
