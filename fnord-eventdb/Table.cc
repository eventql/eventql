/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <thread>
#include <fnord-eventdb/Table.h>
#include <fnord-base/logging.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-msg/MessageEncoder.h>
#include <fnord-msg/MessagePrinter.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "fnord-cstable/CSTableWriter.h"
#include "fnord-cstable/CSTableReader.h"
#include "fnord-cstable/CSTableBuilder.h"

namespace fnord {
namespace eventdb {

RefPtr<Table> Table::open(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema) {
  RefPtr<TableSnapshot> head(new TableSnapshot);
  uint64_t seq = 0;

  return new Table(table_name, replica_id, db_path, schema, seq, head);
}

Table::Table(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema,
    uint64_t head_sequence,
    RefPtr<TableSnapshot> snapshot) :
    name_(table_name),
    replica_id_(replica_id),
    db_path_(db_path),
    schema_(schema),
    seq_(head_sequence + 1),
    head_(snapshot) {
  arenas_.emplace_front(new TableArena(seq_, rnd_.hex128()));
}

void Table::addRecords(const Buffer& records) {
  for (size_t offset = 0; offset < records.size(); ) {
    msg::MessageObject msg;
    msg::MessageDecoder::decode(records, schema_, &msg, &offset);
    addRecord(msg);
  }
}

void Table::addRecord(const msg::MessageObject& record) {
  std::unique_lock<std::mutex> lk(mutex_);
  arenas_.front()->addRecord(record);
  ++seq_;
}

size_t Table::commit() {
  std::unique_lock<std::mutex> lk(mutex_);
  auto arena = arenas_.front();
  const auto& records = arena->records();

  if (records.size() == 0) {
    return 0;
  }

  arenas_.emplace_front(new TableArena(seq_, rnd_.hex128()));
  lk.unlock();

  auto t = std::thread(std::bind(&Table::commitTable, this, arena));
  t.detach();

  return records.size();
}

void Table::commitTable(RefPtr<TableArena> arena) {
  TableChunkRef chunk;
  chunk.replica_id = replica_id_;
  chunk.chunk_id = arena->chunkID();
  chunk.start_sequence = arena->startSequence();
  chunk.num_records = arena->records().size();

  auto filename = StringUtil::format(
      "$0/$1.$2.$3",
      db_path_,
      name_,
      replica_id_,
      chunk.chunk_id);

  fnord::logInfo(
      "fnord.evdb",
      "Writing output sstable: $0",
      filename + ".sst");

  auto sstable = sstable::SSTableWriter::create(
      filename + ".sst~",
      sstable::IndexProvider{},
      nullptr,
      0);

  fnord::logInfo(
      "fnord.evdb",
      "Writing output cstable: $0",
      filename + ".cst");

  cstable::CSTableBuilder cstable(&schema_);

  uint64_t seq = chunk.start_sequence;
  for (const auto& r : arena->records()) {
    Buffer buf;
    msg::MessageEncoder::encode(r, schema_, &buf);
    sstable->appendRow(&seq, sizeof(seq), buf.data(), buf.size());
    cstable.addRecord(r);
    ++seq;
  }

  sstable->finalize();
  cstable.write(filename + ".cst~");

  FileUtil::mv(filename + ".sst~", filename + ".sst");
  FileUtil::mv(filename + ".cst~", filename + ".cst");

  addChunk(chunk);
}

void Table::addChunk(TableChunkRef chunk) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto next = head_->clone();
  next->generation++;
  next->chunks.emplace_back(chunk);
  head_ = next;
}

const String& Table::name() const {
  return name_;
}

RefPtr<TableSnapshot> TableSnapshot::clone() const {
  RefPtr<TableSnapshot> c(new TableSnapshot);
  c->table_name = table_name;
  c->generation = generation;
  c->chunks = chunks;
  return c;
}


} // namespace eventdb
} // namespace fnord

