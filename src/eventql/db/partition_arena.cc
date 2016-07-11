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
#include <eventql/db/partition_arena.h>
#include <eventql/db/LSMTableIndex.h>
#include <eventql/util/protobuf/MessageDecoder.h>

namespace eventql {

PartitionArena::PartitionArena(
    const msg::MessageSchema& schema) :
    schema_(schema),
    num_records_(0),
    cstable_schema_(cstable::TableSchema::fromProtobuf(schema)),
    cstable_schema_ext_(cstable_schema_),
    opened_(false) {
  cstable_schema_ext_.addBool("__lsm_is_update", false);
  cstable_schema_ext_.addBool("__lsm_skip", false);
  cstable_schema_ext_.addString("__lsm_id", false);
  cstable_schema_ext_.addUnsignedInteger("__lsm_version", false);
  cstable_schema_ext_.addUnsignedInteger("__lsm_sequence", false);
}

void PartitionArena::open() {
  cstable_file_.reset(
      new cstable::CSTableFile(
          cstable::BinaryFormatVersion::v0_2_0,
          cstable_schema_ext_));

  cstable_writer_ = cstable::CSTableWriter::openFile(cstable_file_.get());

  is_update_col_ = cstable_writer_->getColumnWriter("__lsm_is_update");
  id_col_ = cstable_writer_->getColumnWriter("__lsm_id");
  version_col_ = cstable_writer_->getColumnWriter("__lsm_version");

  shredder_.reset(
      new cstable::RecordShredder(cstable_writer_.get(), &cstable_schema_));

  opened_ = true;
}

bool PartitionArena::insertRecord(const RecordRef& record) {
  msg::MessageObject obj;
  msg::MessageDecoder::decode(record.record, schema_, &obj);
  insertRecord(record.record_id, record.record_version, obj, record.is_update);
  return true;
}

bool PartitionArena::insertRecord(
    const SHA1Hash& record_id,
    uint64_t record_version,
    const msg::MessageObject& record,
    bool is_update) {
  ScopedLock<std::mutex> lk(mutex_);

  if (!opened_) {
    open();
  }

  auto old = record_versions_.find(record_id);
  if (old == record_versions_.end()) {
    // record does not exist in arena
  } else if (old->second.version < record_version) {
    // record does exist in arena, but the one we're inserting is newer
    skiplist_[old->second.position] = true;
  } else {
    // record in arena is newer or equal to the one we're inserting, return false
    return false;
  }

  {
    shredder_->addRecordFromProtobuf(record, schema_);
    is_update_col_->writeBoolean(0, 0, is_update);
    String id_str((const char*) record_id.data(), record_id.size());
    id_col_->writeString(0, 0, id_str);
    version_col_->writeUnsignedInt(0, 0, record_version);
  }

  RecordVersion rversion;
  rversion.version = record_version;
  rversion.position = num_records_++;
  record_versions_[record_id] = rversion;
  vmap_[record_id] = record_version;
  skiplist_.push_back(false);

  cstable_writer_->commit();
  return true;
}

uint64_t PartitionArena::fetchRecordVersion(const SHA1Hash& record_id) {
  ScopedLock<std::mutex> lk(mutex_);
  auto rec = record_versions_.find(record_id);
  if (rec == record_versions_.end()) {
    return 0;
  } else {
    return rec->second.version;
  }
}

size_t PartitionArena::size() const {
  ScopedLock<std::mutex> lk(mutex_);
  return num_records_;
}

Status PartitionArena::writeToDisk(
    const String& filename,
    uint64_t sequence) {
  if (!opened_) {
    return Status(eRuntimeError, "partition arena is empty");
  }

  auto sequence_col = cstable_writer_->getColumnWriter("__lsm_sequence");
  auto skip_col = cstable_writer_->getColumnWriter("__lsm_skip");
  for (size_t i = 0; i < num_records_; ++i) {
    sequence_col->writeUnsignedInt(0, 0, sequence++);
    skip_col->writeBoolean(0, 0, skiplist_[i]);
  }

  cstable_writer_->commit();
  auto file = File::openFile(filename + ".cst", File::O_WRITE | File::O_CREATE);
  cstable_file_->writeFile(file.fd());

  LSMTableIndex::write(vmap_, filename + ".idx");
  return Status::success();
}

cstable::CSTableFile* PartitionArena::getCSTableFile() const {
  return cstable_file_.get();
}

PartitionArena::SkiplistReader PartitionArena::getSkiplistReader() const {
  ScopedLock<std::mutex> lk(mutex_);
  SkiplistReader reader;
  reader.position = 0;
  reader.skiplist = skiplist_;
  return reader;
}

bool PartitionArena::SkiplistReader::readNext() {
  return skiplist[position++];
}

size_t PartitionArena::SkiplistReader::size() const {
  return skiplist.size();
}

} // namespace eventql

