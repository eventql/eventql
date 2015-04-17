/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEREJANITOR_H
#define _FNORD_EVENTDB_TABLEREJANITOR_H
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-eventdb/TableRepository.h>

namespace fnord {
namespace eventdb {

class TableJanitor {
public:

  TableJanitor(TableRepository* repo);
  void start();
  void stop();
  void check();

protected:
  void run();

  TableRepository* repo_;
  uint64_t interval_;
  std::atomic<bool> running_;
  std::thread thread_;
};

} // namespace eventdb
} // namespace fnord

#endif
