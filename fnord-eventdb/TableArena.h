/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEARENA_H
#define _FNORD_EVENTDB_TABLEARENA_H
#include <fnord-base/stdtypes.h>

namespace fnord {
namespace eventdb {

class TableArena {
public:

  TableArena(
      const String& table_name,
      uint64_t hostid,
      uint64_t offset,
      msg::MessageSchema* schema);

  void addRecord(const msg::MessageObject& record);

  void commit();

};

} // namespace eventdb
} // namespace fnord

#endif
