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
#include "eventql/db/server_allocator.h"
#include "eventql/util/random.h"

namespace eventql {

ServerAllocator::ServerAllocator(ConfigDirectory* cdir) {
  cdir->setServerConfigChangeCallback(
      std::bind(
          &ServerAllocator::updateServerSlot,
          this,
          std::placeholders::_1));

  auto servers = cdir->listServers();
  for (const auto& s : servers) {
    updateServerSlot(s);
  }
}

void ServerAllocator::updateServerSlot(const ServerConfig& s) {
  std::unique_lock<std::mutex> lk(mutex_);
  const auto& sstats = s.server_stats();

  bool is_eligible =
    s.server_status() == SERVER_UP &&
    !s.is_dead() &&
    !s.is_leaving();

  if (is_eligible) {
    backup_servers_.insert(s.server_id());
  } else {
    backup_servers_.erase(s.server_id());
  }

  if (s.server_status() == SERVER_UP && sstats.has_load_factor()) {
    auto& slot = primary_servers_[s.server_id()];
    slot.load_factor = sstats.load_factor();
    slot.disk_free = sstats.disk_available();
    slot.partitions_loading = 0;
    if (sstats.partitions_assigned() > sstats.partitions_loaded()) {
      slot.partitions_loading =
          sstats.partitions_assigned() - sstats.partitions_loaded();
    }
  } else {
    primary_servers_.erase(s.server_id());
  }
}

Status ServerAllocator::allocateStable(
    AllocationPolicy policy,
    const SHA1Hash& token,
    size_t num_servers,
    const Set<String>& exclude_servers,
    Vector<String>* out) const {
  std::vector<std::string> all_servers;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    for (const auto& s : backup_servers_) {
      all_servers.emplace_back(s);
    }
  }

  size_t num_alloced = 0;
  std::sort(
      all_servers.begin(),
      all_servers.end(),
      [] (const std::string& a, const std::string& b) {
        return SHA1::compare(
            SHA1::compute(a),
            SHA1::compute(b)) < 0;
      });

  uint64_t idx = 0;
  while (idx < all_servers.size()) {
    auto cmp = SHA1::compare(
        SHA1::compute(all_servers[idx]),
        token);

    if (cmp < 0) {
      ++idx;
    } else {
      break;
    }
  }

  for (int i = 0; i < all_servers.size(); ++i) {
    const auto& s = all_servers[++idx % all_servers.size()];

    if (exclude_servers.count(s) > 0) {
      continue;
    }

    out->emplace_back(s);
    if (++num_alloced == num_servers) {
      break;
    }
  }

  if (num_alloced >= num_servers) {
    return Status::success();
  } else {
    return Status(eRuntimeError, "not enough live servers");
  }
}

} // namespace eventql

