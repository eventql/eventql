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

CSTableIndex::CSTableIndex(
    const String& db_path) :
    db_path_(db_path) {}

Option<RefPtr<VFSFile>> CSTableIndex::fetchCSTable(
    const String& tsdb_namespace,
    const String& table,
    const SHA1Hash& partition) const {
  auto filepath = FileUtil::joinPaths(
      db_path_,
      StringUtil::format(
          "$0/$1/$2/_cstable",
          tsdb_namespace,
          SHA1::compute(table).toString(),
          partition.toString()));

  if (FileUtil::exists(filepath)) {
    return Some<RefPtr<VFSFile>>(
        new io::MmappedFile(File::openFile(filepath, File::O_READ)));
  } else {
    return None<RefPtr<VFSFile>>();
  }
}

void CSTableIndex::buildCSTable(RefPtr<Partition> partition) {
  auto snap = partition->getSnapshot();
  auto table = partition->getTable();
  auto schema = table->schema();

  logDebug(
      "tsdb",
      "Building CSTable index for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  auto tmppath = filepath + "." + Random::singleton()->hex128();

  cstable::CSTableBuilder cstable(schema.get());

  auto reader = partition->getReader();
  reader->fetchRecords([&schema, &cstable] (const Buffer& record) {
      msg::MessageObject obj;
      msg::MessageDecoder::decode(record.data(), record.size(), *schema, &obj);
      cstable.addRecord(obj);
  });

  cstable.write(tmppath);
  FileUtil::mv(tmppath, filepath);
}

} // namespace tsdb
