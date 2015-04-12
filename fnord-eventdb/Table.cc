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
#include <fnord-base/util/binarymessagewriter.h>
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
  uint64_t head_gen = 0;
  auto gen_prefix = StringUtil::format("$0.$1.", table_name, replica_id);
  FileUtil::ls(db_path, [gen_prefix, &head_gen] (const String& file) -> bool {
    if (StringUtil::beginsWith(file, gen_prefix) &&
        StringUtil::endsWith(file, ".idx")) {
      auto s = file.substr(gen_prefix.size());
      auto gen = std::stoul(s.substr(0, s.size() - 4));

      if (gen > head_gen) {
        head_gen = gen;
      }
    }

    return true;
  });

  RefPtr<TableGeneration> head(new TableGeneration);
  head->table_name = table_name;
  head->generation = head_gen;

  if (head_gen > 0) {
    auto file = FileUtil::read(StringUtil::format(
        "$0/$1.$2.$3.idx",
        db_path,
        table_name,
        replica_id,
        head_gen));

    head->decode(file);
  }

  uint64_t last_seq = 0;
  for (const auto& c : head->chunks) {
    auto l = c.start_sequence + c.num_records;
    if (l > last_seq) {
      last_seq = l;
    }
  }

  return new Table(table_name, replica_id, db_path, schema, last_seq, head);
}

Table::Table(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema,
    uint64_t head_sequence,
    RefPtr<TableGeneration> snapshot) :
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

  auto t = std::thread(std::bind(&Table::writeTable, this, arena));
  t.detach();

  return records.size();
}

void Table::writeTable(RefPtr<TableArena> arena) {
  TableChunkRef chunk;
  chunk.replica_id = replica_id_;
  chunk.chunk_id = arena->chunkID();
  chunk.start_sequence = arena->startSequence();
  chunk.num_records = arena->records().size();

  auto chunkname  = StringUtil::format(
      "$0.$1.$2",
      name_,
      replica_id_,
      chunk.chunk_id);

  fnord::logInfo("fnord.evdb", "Writing chunk: $0", chunkname);
  auto filename = FileUtil::joinPaths(db_path_, chunkname);

  {
    cstable::CSTableBuilder cstable(&schema_);
    auto sstable = sstable::SSTableWriter::create(
        filename + ".sst~",
        sstable::IndexProvider{},
        nullptr,
        0);

    uint64_t seq = chunk.start_sequence;
    for (const auto& r : arena->records()) {
      cstable.addRecord(r);
      Buffer buf;
      msg::MessageEncoder::encode(r, schema_, &buf);
      sstable->appendRow(&seq, sizeof(seq), buf.data(), buf.size());
      ++seq;
    }

    cstable.write(filename + ".cst~");
    sstable->finalize();
  }

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

  writeSnapshot();
}

// precondition: mutex_ must be locked
void Table::writeSnapshot() {
  auto snapname = StringUtil::format(
    "$0.$1.$2",
    name_,
    replica_id_,
    head_->generation);

  fnord::logInfo("fnord.evdb", "Writing snapshot: $0", snapname);
  auto filename = FileUtil::joinPaths(db_path_, snapname + ".idx");

  auto file = File::openFile(filename + "~", File::O_CREATE | File::O_WRITE);
  Buffer buf;
  head_->encode(&buf);
  file.write(buf);

  FileUtil::mv(filename + "~", filename);
}

const String& Table::name() const {
  return name_;
}

RefPtr<TableSnapshot> Table::getSnapshot() {
  std::unique_lock<std::mutex> lk(mutex_);
  return new TableSnapshot(head_, arenas_);
}

RefPtr<TableGeneration> TableGeneration::clone() const {
  RefPtr<TableGeneration> c(new TableGeneration);
  c->table_name = table_name;
  c->generation = generation;
  c->chunks = chunks;
  return c;
}

void TableGeneration::encode(Buffer* buf) {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0x01);
  writer.appendUInt64(generation);
  writer.appendUInt32(table_name.size());
  writer.append(table_name.data(), table_name.size());
  writer.appendUInt32(chunks.size());

  for (const auto& c : chunks) {
    writer.appendUInt16(c.replica_id.size());
    writer.append(c.replica_id.data(), c.replica_id.size());
    writer.appendUInt16(c.chunk_id.size());
    writer.append(c.chunk_id.data(), c.chunk_id.size());
    writer.appendUInt64(c.start_sequence);
    writer.appendUInt64(c.num_records);
  }

  buf->append(writer.data(), writer.size());
}

TableSnapshot::TableSnapshot(
    RefPtr<TableGeneration> _head,
    List<RefPtr<TableArena>> _arenas) :
    head(_head),
    arenas(_arenas) {}



} // namespace eventdb
} // namespace fnord

