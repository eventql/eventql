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
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-msg/MessageEncoder.h>
#include <fnord-msg/MessagePrinter.h>


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

  uint64_t last_seq = 1;
  for (const auto& c : head->chunks) {
    if (c.replica_id != replica_id) {
      continue;
    }

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
    seq_(head_sequence),
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

  fnord::logInfo("fnord.evdb", "Commiting table: $0", name_);

  arenas_.emplace_front(new TableArena(seq_, rnd_.hex128()));
  lk.unlock();

  auto t = std::thread(std::bind(&Table::writeTable, this, arena));
  t.detach();

  return records.size();
}

void Table::merge() {
  merge(1024 * 1024 * 90, 1024 * 1024 * 100);
  merge(1024 * 1024 * 1, 1024 * 1024 * 10);
}

void Table::merge(size_t min_chunk_size, size_t max_chunk_size) {
  std::unique_lock<std::mutex> merge_lk(merge_mutex_);

  auto snap = getSnapshot();
  Vector<TableChunkRef> chunks;
  for (const auto& c : snap->head->chunks) {
    if (c.replica_id == replica_id_) {
      chunks.emplace_back(c);
    }
  }

  std::sort(chunks.begin(), chunks.end(), [] (
      const TableChunkRef& a,
      const TableChunkRef& b) -> bool {
    return a.start_sequence < b.start_sequence;
  });

  Vector<TableChunkRef> input_chunks;
  Set<String> input_chunk_ids;
  size_t cumul_size = 0;
  size_t cumul_recs = 0;
  for (int i = 0; i < chunks.size(); ++i) {
    const auto& c = chunks[i];
    auto cfile = StringUtil::format(
        "$0/$1.$2.$3",
        db_path_,
        name_,
        c.replica_id,
        c.chunk_id);

    auto csize = FileUtil::size(cfile + ".sst");

    if (csize > max_chunk_size) {
      if (input_chunks.empty()) {
        continue;
      } else {
        break;
      }
    }

    if ((cumul_size + csize) > max_chunk_size) {
      break;
    }

    if (!input_chunks.empty() && c.start_sequence != (
          input_chunks.back().start_sequence +
          input_chunks.back().num_records)) {
      break;
    }

    input_chunks.emplace_back(c);
    input_chunk_ids.emplace(c.chunk_id);
    cumul_size += csize;
    cumul_recs += c.num_records;
  }

  if (input_chunks.size() < 2 || cumul_size < min_chunk_size) {
    return;
  }

  TableChunkRef output_chunk;
  output_chunk.replica_id = replica_id_;
  output_chunk.chunk_id = rnd_.hex128();
  output_chunk.start_sequence = input_chunks.front().start_sequence;
  output_chunk.num_records = cumul_recs;

  fnord::logInfo(
      "fnord.evdb",
      "Merging table '$0'\n    input_chunks=$1\n    ouput_chunk=$2",
      name_,
      inspect(input_chunk_ids),
      output_chunk.chunk_id);

  {
    TableChunkMerge mergeop(
        db_path_,
        name_,
        &schema_,
        input_chunks,
        &output_chunk);

    mergeop.merge();
  }

  std::unique_lock<std::mutex> lk(mutex_);

  auto next = head_->clone();
  next->generation++;
  next->chunks.emplace_back(output_chunk);
  for (auto cur = next->chunks.begin(); cur != next->chunks.end(); ) {
    if (input_chunk_ids.count(cur->chunk_id) > 0) {
      input_chunk_ids.erase(cur->chunk_id);
      cur = next->chunks.erase(cur);
    } else {
      ++cur;
    }
  }

  if (input_chunk_ids.size() > 0) {
    fnord::logInfo("fnord.evdb", "Aborting merging for table '$0'", name_);
    // FIXPAUL delete orphaned files...
    return;
  }

  head_ = next;
  writeSnapshot();
}

void Table::gc(size_t keep_generations /* = 2 */) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto head_gen = head_->generation;
  lk.unlock();

  if (head_gen < keep_generations) {
    return;
  }

  Set<String> delete_chunk_ids;
  Set<String> delete_files;

  for (uint64_t gen = head_gen - keep_generations; gen > 0; --gen) {
    auto genfile = StringUtil::format(
        "$0/$1.$2.$3.idx",
        db_path_,
        name_,
        replica_id_,
        gen);

    if (!FileUtil::exists(genfile)) {
      break;
    }

    TableGeneration g;
    g.decode(FileUtil::read(genfile));

    for (const auto& c : g.chunks) {
      delete_chunk_ids.emplace(c.chunk_id);
    }

    delete_files.emplace(genfile);
  }

  for (uint64_t gen = head_gen; gen > (head_gen - keep_generations); --gen) {
    auto genfile = StringUtil::format(
      "$0/$1.$2.$3.idx",
      db_path_,
      name_,
      replica_id_,
      gen);

    TableGeneration g;
    g.decode(FileUtil::read(genfile));

    for (const auto& c : g.chunks) {
      delete_chunk_ids.erase(c.chunk_id);
    }
  }

  for (const auto& c : delete_chunk_ids) {
    auto chunkfile = StringUtil::format(
        "$0/$1.$2.$3",
        db_path_,
        name_,
        replica_id_,
        c);

    delete_files.emplace(chunkfile + ".sst");
    delete_files.emplace(chunkfile + ".cst");
    delete_files.emplace(chunkfile + ".idx");
  }

  for (const auto& f : delete_files) {
    fnord::logInfo("fnord.evdb", "Deleting file: $0", f);
    FileUtil::rm(f);
  }
}

void Table::writeTable(RefPtr<TableArena> arena) {
  TableChunkRef chunk;
  chunk.replica_id = replica_id_;
  chunk.chunk_id = arena->chunkID();
  chunk.start_sequence = arena->startSequence();
  chunk.num_records = arena->records().size();

  {
    TableChunkWriter writer(db_path_, name_, &schema_, &chunk);

    for (const auto& r : arena->records()) {
      writer.addRecord(r);
    }

    writer.commit();
  }

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

TableGeneration::TableGeneration() : generation(0) {}

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
  writer.appendUInt16(table_name.size());
  writer.append(table_name.data(), table_name.size());
  writer.appendUInt32(chunks.size());

  for (const auto& c : chunks) {
    writer.appendUInt16(c.replica_id.size());
    writer.append(c.replica_id.data(), c.replica_id.size());
    writer.appendUInt16(c.chunk_id.size());
    writer.append(c.chunk_id.data(), c.chunk_id.size());
    writer.appendUInt64(c.start_sequence);
    writer.appendUInt64(c.num_records);
    writer.appendUInt64(c.sstable_checksum);
    writer.appendUInt64(c.cstable_checksum);
    writer.appendUInt64(c.index_checksum);
  }

  buf->append(writer.data(), writer.size());
}

void TableGeneration::decode(const Buffer& buf) {
  util::BinaryMessageReader reader(buf.data(), buf.size());
  auto v = *reader.readUInt8();

  auto gen = *reader.readUInt64();
  if (generation > 0 && generation != gen) {
    RAISEF(kRuntimeError, "generation mismatch. $0 vs $1", gen, generation);
  }

  auto tblname_len = *reader.readUInt16();
  String tblname((const char*) reader.read(tblname_len), tblname_len);
  if (table_name.length() > 0 && tblname != table_name) {
    RAISEF(kRuntimeError, "name mismatch. $0 vs $1", tblname, table_name);
  }

  auto nchunks = *reader.readUInt32();
  for (int i = 0; i < nchunks; ++i) {
    TableChunkRef chunk;

    auto replicaid_len = *reader.readUInt16();
    chunk.replica_id =
        String((const char*) reader.read(replicaid_len), replicaid_len);

    auto chunkid_len = *reader.readUInt16();
    chunk.chunk_id =
        String((const char*) reader.read(chunkid_len), chunkid_len);

    chunk.start_sequence = *reader.readUInt64();
    chunk.num_records = *reader.readUInt64();
    chunk.sstable_checksum = *reader.readUInt64();
    chunk.cstable_checksum = *reader.readUInt64();
    chunk.index_checksum = *reader.readUInt64();

    chunks.emplace_back(chunk);
  }
}

TableSnapshot::TableSnapshot(
    RefPtr<TableGeneration> _head,
    List<RefPtr<TableArena>> _arenas) :
    head(_head),
    arenas(_arenas) {}

TableChunkWriter::TableChunkWriter(
    const String& db_path,
    const String& table_name,
    msg::MessageSchema* schema,
    TableChunkRef* chunk) :
    schema_(schema),
    chunk_(chunk),
    seq_(chunk->start_sequence),
    chunk_name_(StringUtil::format(
        "$0.$1.$2",
        table_name,
        chunk->replica_id,
        chunk->chunk_id)),
    chunk_filename_(FileUtil::joinPaths(db_path, chunk_name_)),
    sstable_(sstable::SSTableWriter::create(
        chunk_filename_ + ".sst~",
        sstable::IndexProvider{},
        nullptr,
        0)),
    cstable_(new cstable::CSTableBuilder(schema)) {}

void TableChunkWriter::addRecord(const msg::MessageObject& record) {
  Buffer buf;
  msg::MessageEncoder::encode(record, *schema_, &buf);

  cstable_->addRecord(record);
  sstable_->appendRow(&seq_, sizeof(seq_), buf.data(), buf.size());

  ++seq_;
}

void TableChunkWriter::commit() {
  fnord::logInfo("fnord.evdb", "Writing chunk: $0", chunk_name_);

  cstable_->write(chunk_filename_ + ".cst~");
  sstable_->finalize();

  chunk_->sstable_checksum = FileUtil::checksum(chunk_filename_ + ".sst~");
  chunk_->cstable_checksum = FileUtil::checksum(chunk_filename_ + ".cst~");
  chunk_->index_checksum = 0;

  FileUtil::mv(chunk_filename_ + ".sst~", chunk_filename_ + ".sst");
  FileUtil::mv(chunk_filename_ + ".cst~", chunk_filename_ + ".cst");
}

TableChunkMerge::TableChunkMerge(
    const String& db_path,
    const String& table_name,
    msg::MessageSchema* schema,
    const Vector<TableChunkRef> input_chunks,
    TableChunkRef* output_chunk) :
    db_path_(db_path),
    table_name_(table_name),
    schema_(schema),
    writer_(db_path, table_name, schema, output_chunk),
    input_chunks_(input_chunks) {}

void TableChunkMerge::merge() {
  for (const auto& c : input_chunks_) {
    readTable(StringUtil::format(
        "$0/$1.$2.$3.sst",
        db_path_,
        table_name_,
        c.replica_id,
        c.chunk_id));
  }

  writer_.commit();
}

void TableChunkMerge::readTable(const String& filename) {
  sstable::SSTableReader reader(File::openFile(filename, File::O_READ));

  if (!reader.isFinalized()) {
    RAISEF(kRuntimeError, "unfinished table chunk: $0", filename);
  }

  auto body_size = reader.bodySize();
  if (body_size == 0) {
    fnord::logWarning("fn.evdb", "empty table chunk: $0", filename);
    return;
  }

  auto cursor = reader.getCursor();

  while (cursor->valid()) {
    auto buf = cursor->getDataBuffer();

    msg::MessageObject record;
    msg::MessageDecoder::decode(buf, *schema_, &record);

    writer_.addRecord(record);

    if (!cursor->next()) {
      break;
    }
  }
}

} // namespace eventdb
} // namespace fnord

