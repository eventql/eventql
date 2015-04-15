/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEREPLICATION_H
#define _FNORD_EVENTDB_TABLEREPLICATION_H
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-base/uri.h>
#include <fnord-eventdb/TableRepository.h>

namespace fnord {
namespace eventdb {

class TableReplication {
public:

  TableReplication();

  void start();
  void stop();

  void replicateTableFrom(
      RefPtr<TableWriter> table,
      const URI& source_uri);

protected:
  void pullAll();
  void run();

  uint64_t interval_;
  std::atomic<bool> running_;
  std::thread thread_;

  Vector<Pair<RefPtr<TableWriter>, URI>> targets_;
};

} // namespace eventdb
} // namespace fnord

#endif
