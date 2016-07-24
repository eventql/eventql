/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/io/cstable/cstable_reader.h>
#include <eventql/io/cstable/RecordMaterializer.h>
#include <eventql/db/partition_reader.h>
#include <eventql/db/table.h>

#include "eventql/eventql.h"

namespace eventql {

LSMPartitionReader::LSMPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

// FIXME read in memory data
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
    RefPtr<cstable::ColumnReader> skip_col;
    if (tbl->has_skiplist()) {
      skip_col = cstable->getColumnReader("__lsm_skip");
    }

    auto nrecs = cstable->numRecords();
    for (size_t i = 0; i < nrecs; ++i) {
      uint64_t rlvl;
      uint64_t dlvl;

      bool is_update;
      is_update_col->readBoolean(&rlvl, &dlvl, &is_update);

      String id_str;
      id_col->readString(&rlvl, &dlvl, &id_str);
      SHA1Hash id(id_str.data(), id_str.size());

      bool skip = false;
      if (skip_col.get()) {
        skip_col->readBoolean(&rlvl, &dlvl, &skip);
      }

      if (skip || id_set.count(id) > 0) {
        materializer.skipRecord();
      } else {
        if (is_update) {
          id_set.emplace(id);
        }

        msg::MessageObject record;
        materializer.nextRecord(&record);
        fn(record);
      }
    }
  }
}

Status LSMPartitionReader::findMedianValue(
    const String& column,
    Function<bool (const String& a, const String b)> comparator,
    String* min,
    String* midpoint,
    String* max) {
  auto schema = table_->schema();
  const auto& tables = snap_->state.lsm_tables();
  Set<SHA1Hash> id_set;
  Vector<String> values;
  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    auto cstable_file = FileUtil::joinPaths(
        snap_->base_path,
        tbl->filename() + ".cst");

    if (!FileUtil::exists(cstable_file)) {
      continue;
    }

    auto cstable = cstable::CSTableReader::openFile(cstable_file);
    auto id_col = cstable->getColumnReader("__lsm_id");
    auto is_update_col = cstable->getColumnReader("__lsm_is_update");
    auto value_col = cstable->getColumnReader(column);

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

        String value_str;
        value_col->readString(&rlvl, &dlvl, &value_str);
        values.emplace_back(value_str);
      } else {
        value_col->skipValue();
      }
    }
  }

  if (values.size() == 0) {
    return Status(eRuntimeError, "can't calculate median for empty set");
  }

  std::sort(values.begin(), values.end(), comparator);

  *min = values.front();
  *midpoint = values[values.size() / 2];
  *max = values.back();
  return Status::success();
}

SHA1Hash LSMPartitionReader::version() const {
  return SHA1::compute(StringUtil::toString(snap_->state.lsm_sequence())); // FIXME include arenas?
}

} // namespace tdsb

