/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/stdtypes.h>
#include <fnord/logging.h>
#include <fnord/io/mmappedfile.h>
#include <tsdb/CSTableIndex.h>
#include <tsdb/RecordSet.h>
#include <fnord/protobuf/MessageDecoder.h>
#include <cstable/CSTableBuilder.h>

using namespace fnord;

namespace tsdb {

CSTableIndex::CSTableIndex(
    const TSDBTableScanSpec params,
    msg::MessageSchemaRepository* repo,
    tsdb::TSDBClient* tsdb) :
    params_(params),
    schema_(repo->getSchema(params.schema_name())),
    tsdb_(tsdb) {}

RefPtr<VFSFile> CSTableIndex::computeBlob(dproc::TaskContext* context) {
  fnord::logDebug(
      "fnord.tsdb",
      "Building cstable: stream=$0 partition=$1 schema=$2",
      params_.stream_key(),
      params_.partition_key(),
      params_.schema_name());

  cstable::CSTableBuilder cstable(schema_.get());

  tsdb_->fetchPartition(
      params_.tsdb_namespace(),
      params_.stream_key(),
      SHA1Hash::fromHexString(params_.partition_key()),
      [this, context, &cstable] (const Buffer& buf) {
    if (context->isCancelled()) {
      RAISE(kRuntimeError, "task cancelled");
    }

    msg::MessageObject obj;
    msg::MessageDecoder::decode(buf, *schema_, &obj);
    cstable.addRecord(obj);
  });

  auto tmpfile = "/tmp/__cstableindex_ "+ rnd_.hex64() + ".cst"; // FIXPAUL!
  cstable.write(tmpfile);

  return new io::MmappedFile(
      File::openFile(tmpfile, File::O_READ | File::O_AUTODELETE));
}

Option<String> CSTableIndex::cacheKey() const {
  return Some(
      StringUtil::format(
          "tsdb.cstableindex~$0~$1~$2",
          params_.stream_key(),
          params_.partition_key(),
          params_.version()));
}

}
