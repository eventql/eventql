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

ServerAllocator::ServerAllocator(
    ConfigDirectory* cdir,
    ProcessConfig* config) :
    load_limit_soft_(config->getDouble("server.load_limit_soft")),
    load_limit_hard_(config->getDouble("server.load_limit_hard")),
    partitions_loading_limit_hard_(
        config->getInt("server.partitions_loading_limit_hard", 0)),
    partitions_loading_limit_soft_(
        config->getInt("server.partitions_loading_limit_soft", 0)) {
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
      !s.is_leaving() &&
      sstats.has_load_factor() &&
      !sstats.noalloc() &&
      sstats.load_factor() < load_limit_hard_;

  if (is_eligible) {
    backup_servers_.insert(s.server_id());
  } else {
    backup_servers_.erase(s.server_id());
  }

  bool is_primary =
      is_eligible &&
      sstats.load_factor() < load_limit_soft_;

  if (is_primary) {
    auto& slot = primary_servers_[s.server_id()];
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

using WeightedServer = std::pair<std::string, uint64_t>;

Status ServerAllocator::allocateServers(
    AllocationPolicy policy,
    size_t num_servers,
    const Set<String>& exclude_servers,
    Vector<String>* out) const {
  size_t num_alloced = 0;
  auto excluded = exclude_servers;

  uint64_t max_loading_partitions = 0;
  switch (policy) {
    case AllocationPolicy::MUST_ALLOCATE:
    case AllocationPolicy::BEST_EFFORT:
      max_loading_partitions = partitions_loading_limit_hard_;
      break;
    case AllocationPolicy::IDLE:
      max_loading_partitions = partitions_loading_limit_soft_;
      break;
  }

  // try allocating from primary servers
  while (num_alloced < num_servers) {
    std::vector<WeightedServer> weighted_servers;
    uint64_t weight = 1;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      for (const auto& s : primary_servers_) {
        if (s.second.partitions_loading >= max_loading_partitions ||
            excluded.count(s.first) > 0) {
          continue;
        }

        weight += s.second.disk_free;
        weighted_servers.emplace_back(s.first, weight);
      }
    }

    if (weighted_servers.empty()) {
      break;
    }

    // pick a random server, considering each servers weight
    uint64_t rnd = Random::singleton()->random64() % weight;
    auto iter = std::lower_bound(
        weighted_servers.begin(),
        weighted_servers.end(),
        rnd,
        [] (const WeightedServer& a, uint64_t b) {
          return a.second < b;
        });

    assert(iter != weighted_servers.end());

    out->emplace_back(iter->first);
    excluded.insert(iter->first);
    ++num_alloced;
  }

  // if we were unable to allocate enough servers from the primary servers and
  // the priority is best effort, bail
  if (num_alloced < num_servers && policy != AllocationPolicy::MUST_ALLOCATE) {
    return Status(eRuntimeError, "best effort alloc failed");
  }

  // pick remainig servers randomly from the backup servers
  if (num_alloced < num_servers) {
    std::vector<std::string> all_servers;
    {
      std::unique_lock<std::mutex> lk(mutex_);
      for (const auto& s : backup_servers_) {
        all_servers.emplace_back(s);
      }
    }

    if (!all_servers.empty()) {
      size_t idx = Random::singleton()->random64() % all_servers.size();
      for (size_t i = 0; i < all_servers.size() && num_alloced < num_servers; ++i) {
        const auto& s = all_servers[++idx % all_servers.size()];

        if (excluded.count(s) > 0) {
          continue;
        }

        out->emplace_back(s);
        excluded.insert(s);
        ++num_alloced;
      }
    }
  }

  if (num_alloced >= num_servers) {
    return Status::success();
  } else {
    return Status(
        eRuntimeError,
        StringUtil::format(
            "not enough live servers (got=$0, required=$1)",
            num_alloced,
            num_servers));
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

  // sort the backup servers by SHA1 hash of their name and then find the
  // first server with a sha1 value that is not less than our token
  std::sort(
      all_servers.begin(),
      all_servers.end(),
      [] (const std::string& a, const std::string& b) {
        return SHA1::compare(
            SHA1::compute(a),
            SHA1::compute(b)) < 0;
      });

  // FIXME this should use std::lower_bound
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

  // return the next N servers
  for (size_t i = 0; i < all_servers.size(); ++i) {
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

