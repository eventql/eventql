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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/db/file_tracker.h>
#include <eventql/config/config_directory.h>
#include "eventql/eventql.h"
#include <thread>
#include <condition_variable>

namespace eventql {

class Leader {
public:

  Leader(
      ConfigDirectory* cdir,
      uint64_t rebalance_interval);

  ~Leader();

  bool runLeaderProcedure();

  void startLeaderThread();
  void stopLeaderThread();

protected:

  ConfigDirectory* cdir_;
  uint64_t rebalance_interval_;
  std::thread thread_;
  bool thread_running_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace eventql

