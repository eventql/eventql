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
#ifndef _STX_COMM_LBGROUP_H
#define _STX_COMM_LBGROUP_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/net/inetaddr.h"

namespace stx {

class ServerGroup {
public:
  ServerGroup();

  std::string getServerForNextRequest();

  void addServer(const std::string& addr);
  void removeServer(const std::string& addr);
  void markServerAsDown(const std::string& addr);

protected:
  enum kServerState {
    S_UP,
    S_DOWN
  };

  struct Server {
    Server(const std::string& addr);
    std::string addr;
    kServerState state;
  };

  /**
   * Should return the index of the picked host
   */
  virtual int pickServerForNextRequest(
      const std::vector<Server>& servers) = 0;

private:
  std::mutex mutex_;
  std::vector<Server> servers_;
};

class RoundRobinServerGroup : public ServerGroup {
public:
  RoundRobinServerGroup();
protected:
  int pickServerForNextRequest(const std::vector<Server>& servers) override;
  unsigned last_index_;
};

}
#endif
