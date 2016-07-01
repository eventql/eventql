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
#include "eventql/eventql.h"
#include "eventql/util/logging.h"
#include <eventql/db/CompactionStrategy.h>
#include <eventql/db/LSMTableIndex.h>
#include <eventql/io/cstable/cstable_writer.h>
#include <eventql/util/io/fileutil.h>

namespace eventql {

SimpleCompactionStrategy::SimpleCompactionStrategy(
    RefPtr<Partition> partition,
    LSMTableIndexCache* idx_cache,
    size_t num_tables_soft_limit,
    size_t num_tables_hard_limit) :
    partition_(partition),
    idx_cache_(idx_cache),
    num_tables_soft_limit_(num_tables_soft_limit),
    num_tables_hard_limit_(num_tables_hard_limit) {}

bool SimpleCompactionStrategy::compact(
    const Vector<LSMTableRef>& input,
    Vector<LSMTableRef>* output) {
  if (input.empty()) {
    return false;
  }

  if (!needsCompaction(input)) {
    return false;
  }

  HashMap<SHA1Hash, uint64_t> vmap;
  for (auto tbl = input.rbegin(); tbl != input.rend(); ++tbl) {
    auto idx = idx_cache_->lookup(
        FileUtil::joinPaths(partition_->getRelativePath(), tbl->filename()));
    idx->list(&vmap);
  }

  auto table = partition_->getTable();
  auto base_path = partition_->getAbsolutePath();
  auto cstable_schema = cstable::TableSchema::fromProtobuf(*table->schema());
  auto cstable_schema_ext = cstable_schema;
  cstable_schema_ext.addBool("__lsm_is_update", false);
  cstable_schema_ext.addString("__lsm_id", false);
  cstable_schema_ext.addUnsignedInteger("__lsm_version", false);
  cstable_schema_ext.addUnsignedInteger("__lsm_sequence", false);

  auto cstable_filename = Random::singleton()->hex64();
  auto cstable_filepath = FileUtil::joinPaths(base_path, cstable_filename);

  auto cstable = cstable::CSTableWriter::createFile(
      cstable_filepath + ".cst",
      cstable::BinaryFormatVersion::v0_1_0,
      cstable_schema_ext);

  auto is_update_col = cstable->getColumnWriter("__lsm_is_update");
  auto id_col = cstable->getColumnWriter("__lsm_id");
  auto version_col = cstable->getColumnWriter("__lsm_version");
  auto sequence_col = cstable->getColumnWriter("__lsm_sequence");

  for (const auto& tbl : input) {
    auto input_cstable_file = FileUtil::joinPaths(
        base_path,
        tbl.filename() + ".cst");

    if (!FileUtil::exists(input_cstable_file)) {
      logWarning("evqld", "missing table file: $0", input_cstable_file);
      continue;
    }

    try {
      auto input_cstable = cstable::CSTableReader::openFile(input_cstable_file);
      auto input_id_col = input_cstable->getColumnReader("__lsm_id");
      auto input_version_col = input_cstable->getColumnReader("__lsm_version");
      auto input_sequence_col = input_cstable->getColumnReader("__lsm_sequence");

      Vector<Pair<
          RefPtr<cstable::ColumnReader>,
          RefPtr<cstable::ColumnWriter>>> columns;

      auto flat_columns = cstable_schema.flatColumns();
      for (const auto& col : flat_columns) {
        if (input_cstable->hasColumn(col.column_name)) {
          columns.emplace_back(
              input_cstable->getColumnReader(col.column_name),
              cstable->getColumnWriter(col.column_name));
        } else {
          if (col.dlevel_max == 0) {
            RAISE(
                kIllegalStateError,
                "can't merge CSTables after a new required column was added");
          }

          columns.emplace_back(
              nullptr,
              cstable->getColumnWriter(col.column_name));
        }
      }

      auto nrecords = input_cstable->numRecords();
      for (size_t i = 0; i < nrecords; ++i) {
        uint64_t rlvl;
        uint64_t dlvl;

        String id_str;
        input_id_col->readString(&rlvl, &dlvl, &id_str);
        SHA1Hash id(id_str.data(), id_str.size());

        uint64_t version;
        input_version_col->readUnsignedInt(&rlvl, &dlvl, &version);

        uint64_t sequence;
        input_sequence_col->readUnsignedInt(&rlvl, &dlvl, &sequence);

        const auto& vmap_iter = vmap.find(id);
        if (vmap_iter == vmap.end()) {
          RAISE(kIllegalStateError, "invalid cstable contents");
        }

        if (version == vmap_iter->second) {
          is_update_col->writeBoolean(0, 0, false);
          id_col->writeString(0, 0, id_str);
          version_col->writeUnsignedInt(0, 0, version);
          sequence_col->writeUnsignedInt(0, 0, sequence);

          for (auto& col : columns) {
            if (col.first.get()) {
              do {
                col.first->copyValue(col.second.get());
              } while (col.first->nextRepetitionLevel() > 0);
            } else {
              col.second->writeNull(0, 0);
            }
          }

          cstable->addRow();
        } else {
          for (auto& col : columns) {
            if (col.first.get()) {
              do {
                col.first->skipValue();
              } while (col.first->nextRepetitionLevel() > 0);
            }
          }
        }
      }
    } catch (const std::exception& e) {
      logError(
          "evqld",
          "error while compacting table: $0 -- $1",
          input_cstable_file,
          e.what());

      throw e;
    }
  }

  cstable->commit();

  OrderedMap<SHA1Hash, uint64_t> vmap_ordered;
  for (const auto& p : vmap) {
    vmap_ordered.emplace(p);
  }
  LSMTableIndex::write(vmap_ordered, cstable_filepath + ".idx");

  LSMTableRef tbl_ref;
  tbl_ref.set_filename(cstable_filename);
  tbl_ref.set_first_sequence(input.begin()->first_sequence());
  tbl_ref.set_last_sequence(input.rbegin()->last_sequence());
  tbl_ref.set_size_bytes(FileUtil::size(cstable_filepath + ".cst"));
  output->emplace_back(tbl_ref);

  return true;
}

bool SimpleCompactionStrategy::needsCompaction(
    const Vector<LSMTableRef>& tables) {
  return tables.size() > num_tables_soft_limit_;
}

bool SimpleCompactionStrategy::needsUrgentCompaction(
    const Vector<LSMTableRef>& tables) {
  return tables.size() > num_tables_hard_limit_;
}

} // namespace eventql

