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
#include "eventql/util/exception.h"
#include "eventql/util/rpc/ServerGroup.h"

namespace stx {

ServerGroup::ServerGroup() {}

std::string ServerGroup::getServerForNextRequest() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (servers_.empty()) {
    RAISE(kRPCError, "lb group is empty, giving up");
  }

  auto picked_index = pickServerForNextRequest(servers_);
  if (picked_index < 0) {
    RAISE(kRPCError, "no available servers for this request, giving up");
  }

  return servers_[picked_index].addr;
}

void ServerGroup::addServer(const std::string& addr) {
  std::unique_lock<std::mutex> lk(mutex_);
  servers_.emplace_back(addr);
}

void ServerGroup::removeServer(const std::string& addr) {
}

void ServerGroup::markServerAsDown(const std::string& addr) {
}

ServerGroup::Server::Server(
    const std::string& _addr) :
    addr(_addr),
    state(S_UP) {}

RoundRobinServerGroup::RoundRobinServerGroup() : last_index_(0) {}

int RoundRobinServerGroup::pickServerForNextRequest(
    const std::vector<Server>& servers) {
  auto s = servers.size();

  for (int i = 0; i < s; ++i) {
    last_index_ = (last_index_ + 1) % s;

    if (servers[last_index_].state == S_UP) {
      return last_index_;
    }
  }

  return -1;
}

}
