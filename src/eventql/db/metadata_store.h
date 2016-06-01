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
#include "eventql/util/stdtypes.h"
#include "eventql/util/status.h"
#include "eventql/util/SHA1.h"
#include "eventql/db/metadata_file.h"

namespace eventql {

class MetadataStore {
public:
  static const constexpr size_t kDefaultMaxEntries = 1024;

  MetadataStore(
      const String& path_prefix,
      size_t cache_maxentries = kDefaultMaxEntries);

  Status getMetadataFile(
      const String& ns,
      const String& table_name,
      const SHA1Hash& txid,
      RefPtr<MetadataFile>* file) const;

  bool hasMetadataFile(
      const String& ns,
      const String& table_name,
      const SHA1Hash& txid);

  Status storeMetadataFile(
      const String& ns,
      const String& table_name,
      const MetadataFile& file);

  size_t getCacheSize() const;

protected:

  struct CacheEntry {
    String key;
    RefPtr<MetadataFile> file;
    CacheEntry* prev;
    CacheEntry* next;
  };

  String getBasePath(
      const String& ns,
      const String& table_name) const;

  String getPath(
      const String& ns,
      const String& table_name,
      const SHA1Hash& txid) const;

  String path_prefix_;
  std::mutex commit_mutex_;
  size_t cache_maxentries_;
  mutable std::mutex cache_mutex_;
  mutable HashMap<String, ScopedPtr<CacheEntry>> cache_idx_;
  mutable CacheEntry* cache_head_;
  mutable CacheEntry* cache_tail_;
  mutable size_t cache_numentries_;
};

} // namespace eventql

