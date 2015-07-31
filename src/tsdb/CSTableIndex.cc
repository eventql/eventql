/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/logging.h>
#include <stx/io/mmappedfile.h>
#include <tsdb/CSTableIndex.h>
#include <tsdb/RecordSet.h>
#include <stx/protobuf/MessageDecoder.h>
#include <cstable/CSTableBuilder.h>

using namespace stx;

namespace tsdb {

Option<RefPtr<VFSFile>> CSTableIndex::fetchCSTable(
    const String& tsdb_namespace,
    const String& table,
    const SHA1Hash& partition) const {
  return None<RefPtr<VFSFile>>();
}

void CSTableIndex::buildCSTable(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> partition) {
  logDebug(
      "tsdb",
      "Building CSTable index for partition $0/$1/$2",
      partition->state.tsdb_namespace(),
      table->name(),
      partition->key.toString());
}


} // namespace tsdb
