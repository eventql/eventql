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

ServerAllocator::ServerAllocator(ConfigDirectory* cdir) : cdir_(cdir) {}

/**
 * Allocation algorithm:
 *
 * - To calculate server loads:
 *   - Count the total number of partitions N_total
 *   - Calculate the total server capacity C_total
 *   - For each server:
 *       - Calculate W_server = C_server / C_total
 *       - Count the used disk space C_server_used
 *       - Count the total assinged partitions N_server
 *       - Count the total assinged partitions that are serving N_server_serving
 *       - Calculate the ratio of serving to total partitions R_serving = N_server_serving / N_server
 *       - Calculate a load factor L_server_assigned = N_server / (N_total * W_server)
 *       - Calculate a load factor L_server_disk = (C_server_used / R_serving) / C_server
 *       - The comined load factor is L_server = max(L_server_assigned, L_server_disk)
 *    - Pick a given server with probability P_server = min(1, log(L_server) * -0.5)
 */
Status ServerAllocator::allocateServers(
    AllocationPolicy policy,
    size_t num_servers,
    const Set<String>& exclude_servers,
    Vector<String>* out) const {
  size_t num_alloced = 0;
  auto all_servers = cdir_->listServers();
  uint64_t idx = Random::singleton()->random64();
  for (int i = 0; i < all_servers.size(); ++i) {
    const auto& s = all_servers[++idx % all_servers.size()];

    if (s.is_dead() ||
        s.is_leaving() ||
        s.allocation_policy() == ALLOC_POLICY_NOALLOC ||
        s.server_status() != SERVER_UP) {
      continue;
    }

    if (exclude_servers.count(s.server_id()) > 0) {
      continue;
    }

    out->emplace_back(s.server_id());
    if (++num_alloced == num_servers) {
      break;
    }
  }

  if (num_alloced < num_servers &&
      policy == MUST_ALLOCATE) {
    return Status(eRuntimeError, "not enough live servers");
  }

  return Status::success();
}

Status ServerAllocator::allocateStable(
    AllocationPolicy policy,
    const SHA1Hash& token,
    size_t num_servers,
    const Set<String>& exclude_servers,
    Vector<String>* out) const {
  size_t num_alloced = 0;
  auto all_servers = cdir_->listServers();
  std::sort(
      all_servers.begin(),
      all_servers.end(),
      [] (const ServerConfig& a, const ServerConfig& b) {
        return SHA1::compare(
            SHA1::compute(a.server_id()),
            SHA1::compute(b.server_id())) < 0;
      });

  uint64_t idx = 0;
  while (idx < all_servers.size()) {
    auto cmp = SHA1::compare(
        SHA1::compute(all_servers[idx].server_id()),
        token);

    if (cmp < 0) {
      ++idx;
    } else {
      break;
    }
  }

  for (int i = 0; i < all_servers.size(); ++i) {
    const auto& s = all_servers[++idx % all_servers.size()];

    if (s.is_dead() ||s.is_leaving() || s.server_status() != SERVER_UP) {
      continue;
    }

    if (exclude_servers.count(s.server_id()) > 0) {
      continue;
    }

    out->emplace_back(s.server_id());
    if (++num_alloced == num_servers) {
      break;
    }
  }

  if (num_alloced < num_servers &&
      policy == MUST_ALLOCATE) {
    return Status(eRuntimeError, "not enough live servers");
  }

  return Status::success();
}

} // namespace eventql

