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
#include <algorithm>
#include "eventql/db/metadata_cache.h"
#include "eventql/util/inspect.h"

namespace eventql {

bool MetadataCache::get(
    const PartitionFindRequest& request,
    PartitionFindResponse* response) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto cache_key = request.db_namespace() + "~" + request.table_id();
  auto cache_iter = cache_.find(cache_key);
  if (cache_iter == cache_.end()) {
    return false;
  }

  auto& entry = cache_iter->second;
  if (entry.sequence_number < request.min_sequence_number()) {
    return false;
  }

  auto ks = request.keyspace();
  auto iter = std::upper_bound(
      entry.partitions.begin(),
      entry.partitions.end(),
      request.key(),
      [ks] (const std::string& a, const CachedPartitionMapEntry& b) {
        return comparePartitionKeys(ks, a, b.end) < 0;
      });

  if (iter == entry.partitions.end() ||
      comparePartitionKeys(ks, iter->begin, request.key()) > 0 ||
      comparePartitionKeys(ks, iter->end, request.key()) <= 0) {
    return false;
  }

  response->set_sequence_number(entry.sequence_number);
  response->set_partition_id(iter->partition_id);
  response->set_partition_keyrange_begin(iter->begin);
  response->set_partition_keyrange_end(iter->end);
  for (const auto& s : iter->servers) {
    response->add_servers_for_insert(s);
  }

  return true;
}

void MetadataCache::store(
    const PartitionFindRequest& request,
    const PartitionFindResponse& response) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto cache_key = request.db_namespace() + "~" + request.table_id();
  auto cache_iter = cache_.find(cache_key);
  if (cache_iter == cache_.end()) {
    CachedPartitionMap e;
    e.sequence_number = response.sequence_number();
    cache_iter = cache_.emplace(cache_key, e).first;
  }

  auto& entry = cache_iter->second;
  if (entry.sequence_number > response.sequence_number()) {
    return;
  }

  if (entry.sequence_number < response.sequence_number()) {
    entry.partitions.clear();
    entry.sequence_number = response.sequence_number();
  }

  CachedPartitionMapEntry p;
  p.partition_id = response.partition_id();
  p.begin = response.partition_keyrange_begin();
  p.end = response.partition_keyrange_end();
  for (const auto& s : response.servers_for_insert()) {
    p.servers.push_back(s);
  }

  auto ks = request.keyspace();
  auto iter = std::lower_bound(
      entry.partitions.begin(),
      entry.partitions.end(),
      p.begin,
      [ks] (const CachedPartitionMapEntry& a, const std::string& b) {
        return comparePartitionKeys(ks, a.begin, b) < 0;
      });

  if (iter != entry.partitions.end() &&
      comparePartitionKeys(ks, iter->begin, p.begin) == 0) {
    return;
  }

  entry.partitions.insert(iter, p);
}

} // namespace eventql

