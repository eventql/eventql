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
#include <stx/io/fileutil.h>
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
  auto filepath = fetchCSTableFilename(tsdb_namespace, table, partition);

  if (filepath.isEmpty()) {
    return None<RefPtr<VFSFile>>();
  } else {
    return Some<RefPtr<VFSFile>>(
        new io::MmappedFile(File::openFile(filepath.get(), File::O_READ)));
  }
}

Option<String> CSTableIndex::fetchCSTableFilename(
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
    return Some(filepath);
  } else {
    return None<String>();
  }
}

void CSTableIndex::buildCSTable(RefPtr<Partition> partition) {
  auto table = partition->getTable();
  if (table->storage() == tsdb::TBL_STORAGE_STATIC) {
    return;
  }

  auto snap = partition->getSnapshot();
  auto schema = table->schema();

  auto metapath = FileUtil::joinPaths(snap->base_path, "_cstable_meta");
  auto metapath_tmp = metapath + "." + Random::singleton()->hex128();
  if (FileUtil::exists(metapath)) {
    auto metadata = FileUtil::read(metapath);
    auto last_offset = std::stoull(metadata.toString());

    if (last_offset >= snap->nrecs) {
      return; // cstable is up to date...
    }
  }

  logDebug(
      "tsdb",
      "Building CSTable index for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  auto filepath_tmp = filepath + "." + Random::singleton()->hex128();

  {
    cstable::CSTableBuilder cstable(schema.get());

    auto reader = partition->getReader();
    reader->fetchRecords([&schema, &cstable] (const Buffer& record) {
        msg::MessageObject obj;
        msg::MessageDecoder::decode(record.data(), record.size(), *schema, &obj);
        cstable.addRecord(obj);
    });

    cstable.write(filepath_tmp);
  }

  {
    auto metafile = File::openFile(
        metapath_tmp,
        File::O_CREATE | File::O_WRITE);

    metafile.write(StringUtil::toString(snap->nrecs));
  }

  FileUtil::mv(filepath_tmp, filepath);
  FileUtil::mv(metapath_tmp, metapath);
}

} // namespace tsdb
