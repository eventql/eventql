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

using namespace fnord;

namespace tsdb {

CSTableIndex::CSTableIndex(
    const TSDBTableScanSpec params,
    tsdb::TSDBNode* tsdb) :
    params_(params),
    tsdb_(tsdb) {}

RefPtr<VFSFile> CSTableIndex::computeBlob(dproc::TaskContext* context) {
  fnord::logDebug(
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

  auto cstable = partition.get()->cstableFile();
  if (cstable.isEmpty()) {
    return new Buffer();
  }

  return cstable.get();
}

}
