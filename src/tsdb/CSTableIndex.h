/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <tsdb/TSDBService.h>
#include <stx/random.h>

using namespace stx;

namespace tsdb {

class CSTableIndex {
public:

  Option<RefPtr<VFSFile>> fetchCSTable(
      const String& tsdb_namespace,
      const String& table,
      const SHA1Hash& partition) const;

  void buildCSTable(
      RefPtr<Table> table,
      RefPtr<PartitionSnapshot> partition);

};

}
