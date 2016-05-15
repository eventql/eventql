/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
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

