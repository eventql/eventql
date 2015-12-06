/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/core/CompactionStrategy.h>
#include <zbase/core/RecordVersionMap.h>
#include <cstable/CSTableWriter.h>
#include <stx/io/FileUtil.h>

using namespace stx;

namespace zbase {

SimpleCompactionStrategy::SimpleCompactionStrategy(
    RefPtr<Table> table,
    const String& base_path) :
    table_(table),
    base_path_(base_path) {}

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
    RecordVersionMap::load(
        &vmap,
        FileUtil::joinPaths(base_path_, tbl->filename() + ".idx"));
  }

  auto cstable_schema = cstable::TableSchema::fromProtobuf(*table_->schema());
  auto cstable_schema_ext = cstable_schema;
  cstable_schema_ext.addBool("__lsm_is_update", false);
  cstable_schema_ext.addString("__lsm_id", false);
  cstable_schema_ext.addUnsignedInteger("__lsm_version", false);
  cstable_schema_ext.addUnsignedInteger("__lsm_sequence", false);

  auto cstable_filename = Random::singleton()->hex64();
  auto cstable_filepath = FileUtil::joinPaths(base_path_, cstable_filename);
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
        base_path_,
        tbl.filename() + ".cst");

    auto input_cstable = cstable::CSTableReader::openFile(input_cstable_file);
    auto input_id_col = input_cstable->getColumnReader("__lsm_id");
    auto input_version_col = input_cstable->getColumnReader("__lsm_version");
    auto input_sequence_col = input_cstable->getColumnReader("__lsm_sequence");

    Vector<Pair<
        RefPtr<cstable::ColumnReader>,
        RefPtr<cstable::ColumnWriter>>> columns;
    for (const auto& col : cstable_schema.columns()) {
      columns.emplace_back(
          input_cstable->getColumnReader(col->name),
          cstable->getColumnWriter(col->name));
    }

    auto nrecords = input_cstable->numRecords();
    for (size_t i = 0; i < nrecords; ++i) {
      uint64_t rlvl;
      uint64_t dlvl;

      String id_str;
      input_id_col->readString(&rlvl, &dlvl, &id_str);
      auto id = SHA1Hash::fromHexString(id_str);

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
          while (!col.first->eofReached()) {
            col.first->copyValue(col.second.get());
            if (col.first->nextRepetitionLevel() == 0) {
              break;
            }
          }
        }

        cstable->addRow();
      } else {
        for (auto& col : columns) {
          while (!col.first->eofReached()) {
            col.first->skipValue();
            if (col.first->nextRepetitionLevel() == 0) {
              break;
            }
          }
        }
      }
    }
  }

  cstable->commit();
  RecordVersionMap::write(vmap, cstable_filepath + ".idx");

  LSMTableRef tbl_ref;
  tbl_ref.set_filename(cstable_filename);
  tbl_ref.set_first_sequence(input.begin()->first_sequence());
  tbl_ref.set_last_sequence(input.rbegin()->last_sequence());
  output->emplace_back(tbl_ref);

  return true;
}

bool SimpleCompactionStrategy::needsCompaction(
    const Vector<LSMTableRef>& tables) {
  return tables.size() > 3;
}

} // namespace zbase

