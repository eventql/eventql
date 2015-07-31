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

/*
CSTableIndex::CSTableIndex(
    const TSDBTableScanSpec params,
    tsdb::TSDBService* tsdb) :
    params_(params),
    tsdb_(tsdb) {}

RefPtr<VFSFile> CSTableIndex::computeBlob(dproc::TaskContext* context) {
  stx::logDebug(
      "fnord.tsdb",
      "Fetching cstable: namespace=$0 table=$1 partition=$2 schema=$2",
      params_.tsdb_namespace(),
      params_.stream_key(),
      params_.partition_key());

  auto partition = tsdb_->findPartition(
      params_.tsdb_namespace(),
      params_.stream_key(),
      SHA1Hash::fromHexString(params_.partition_key()));

  if (partition.isEmpty()) {
    return new Buffer();
  }

  auto partition_reader = partition.get()->getReader();

  auto cstable = partition_reader->fetchSecondaryIndex("cstable");
  if (cstable.isEmpty()) {
    return new Buffer();
  }

  return cstable.get();
}
*/

}
