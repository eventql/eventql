/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLEREPOSITORY_H
#define _FNORD_EVENTDB_TABLEREPOSITORY_H
#include <fnord-base/stdtypes.h>
#include <fnord-eventdb/Table.h>

namespace fnord {
namespace eventdb {

class TableRepository {
public:

  void addTable(RefPtr<Table> table);
  RefPtr<Table> findTable(const String& name) const;

protected:
  HashMap<String, RefPtr<Table>> tables_;
};

} // namespace eventdb
} // namespace fnord

#endif
