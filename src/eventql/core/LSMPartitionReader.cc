/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/fnv.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/infra/cstable/CSTableReader.h>
#include <eventql/infra/cstable/RecordMaterializer.h>
#include <eventql/core/LSMPartitionReader.h>
#include <eventql/core/Table.h>

using namespace stx;

namespace zbase {

LSMPartitionReader::LSMPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

void LSMPartitionReader::fetchRecords(
    const Set<String>& required_columns,
    Function<void (const msg::MessageObject& record)> fn) {
  auto schema = table_->schema();
  const auto& tables = snap_->state.lsm_tables();
  Set<SHA1Hash> id_set;
  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    auto cstable_file = FileUtil::joinPaths(
        snap_->base_path,
        tbl->filename() + ".cst");
    auto cstable = cstable::CSTableReader::openFile(cstable_file);
    cstable::RecordMaterializer materializer(
        schema.get(),
        cstable.get(),
        required_columns);
    auto id_col = cstable->getColumnReader("__lsm_id");
    auto is_update_col = cstable->getColumnReader("__lsm_is_update");

    auto nrecs = cstable->numRecords();
    for (size_t i = 0; i < nrecs; ++i) {
      uint64_t rlvl;
      uint64_t dlvl;

      bool is_update;
      is_update_col->readBoolean(&rlvl, &dlvl, &is_update);

      String id_str;
      id_col->readString(&rlvl, &dlvl, &id_str);
      SHA1Hash id(id_str.data(), id_str.size());

      if (id_set.count(id) == 0) {
        if (is_update) {
          id_set.emplace(id);
        }

        msg::MessageObject record;
        materializer.nextRecord(&record);
        fn(record);
      } else {
        materializer.skipRecord();
      }
    }
  }
}

SHA1Hash LSMPartitionReader::version() const {
  return SHA1::compute(StringUtil::toString(snap_->state.lsm_sequence())); // FIXME include arenas?
}

} // namespace tdsb

