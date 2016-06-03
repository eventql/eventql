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
#include "eventql/db/server_allocator.h"
#include "eventql/util/random.h"

namespace eventql {

ServerAllocator::ServerAllocator(ConfigDirectory* cdir) : cdir_(cdir) {}

Status ServerAllocator::allocateServers(
    size_t num_servers,
    Set<String>* servers) const {
  Vector<String> all_servers;
  for (const auto& s : cdir_->listServers()) {
    if (s.is_dead() || s.is_leaving() || s.server_status() != SERVER_UP) {
      continue;
    }

    all_servers.emplace_back(s.server_id());
  }

  if (all_servers.size() < num_servers) {
    return Status(eRuntimeError, "not enough live servers");
  }

  uint64_t idx = Random::singleton()->random64();
  for (int i = 0; i < num_servers; ++i) {
    servers->emplace(all_servers[++idx % all_servers.size()]);
  }

  return Status::success();
}

} // namespace eventql

