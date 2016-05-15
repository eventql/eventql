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
#include <algorithm>
#include <thread>
#include <fnord-logtable/TableWriter.h>
#include <fnord-logtable/TableChunkSummaryWriter.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/io/FileLock.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/util/protobuf/MessageEncoder.h>
#include <eventql/util/protobuf/MessagePrinter.h>

namespace util {
namespace logtable {

RefPtr<TableWriter> TableWriter::open(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema,
    TaskScheduler* scheduler) {
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

  return new TableWriter(
      table_name,
      replica_id,
      db_path,
      schema,
      last_seq,
      head,
      scheduler);
}

TableWriter::TableWriter(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema,
    uint64_t head_sequence,
    RefPtr<TableGeneration> snapshot,
    TaskScheduler* scheduler) :
    artifacts_(
        db_path,
        StringUtil::format("$0.$1", table_name, replica_id_),
        false),
    name_(table_name),
    replica_id_(replica_id),
    db_path_(db_path),
    schema_(schema),
    scheduler_(scheduler),
    seq_(head_sequence),
    head_(snapshot),
    lock_(StringUtil::format("$0/$1.$2.lck", db_path_, name_, replica_id_)),
    gc_delay_(kMicrosPerSecond * 500) {
  lock_.lock();
  arenas_.emplace_front(new TableArena(seq_, rnd_.hex128()));
}

void TableWriter::addRecord(const Buffer& record) {
  msg::MessageObject msg;
  msg::MessageDecoder::decode(record, schema_, &msg);
  addRecord(msg);
}

void TableWriter::addRecord(const msg::MessageObject& record) {
  std::unique_lock<std::mutex> lk(mutex_);
  arenas_.front()->addRecord(record);
  ++seq_;

  if (arenas_.front()->size() > 10000) { // FIXPAUL!
    commitWithLock();
  }
}

size_t TableWriter::commit() {
  std::unique_lock<std::mutex> lk(mutex_);
  return commitWithLock();
}

size_t TableWriter::commitWithLock() {
  auto arena = arenas_.front();
  const auto& records = arena->records();

  if (records.size() == 0) {
    return 0;
  }

  logInfo("fnord.evdb", "Commiting table: $0", name_);

  arenas_.emplace_front(new TableArena(seq_, rnd_.hex128()));

  scheduler_->run(std::bind(&TableWriter::writeTable, this, arena));

  return records.size();
}

void TableWriter::merge() {
  std::unique_lock<std::mutex> merge_lk(merge_mutex_);

  auto snap = getSnapshot();

  Vector<TableChunkRef> input_chunks;
  TableChunkRef output_chunk;

  if (!merge_policy_.findNextMerge(
        snap,
        db_path_,
        name_,
        replica_id_,
        &input_chunks,
        &output_chunk)) {
    return;
  }

  Set<String> input_chunk_ids;
  for (const auto& c : input_chunks) {
    input_chunk_ids.emplace(c.chunk_id);
  }

  logInfo(
      "fnord.evdb",
      "Merging table '$0'\n    input_chunks=$1\n    ouput_chunk=$2",
      name_,
      inspect(input_chunk_ids),
      output_chunk.chunk_id);

  {
    RefPtr<TableChunkWriter> writer(
        new TableChunkWriter(db_path_, name_, &schema_, &output_chunk));

    for (const auto& summary_factory : summaries_) {
      writer->addSummary(summary_factory());
    }

    TableChunkMerge mergeop(
        db_path_,
        name_,
        &schema_,
        input_chunks,
        &output_chunk,
        writer);

    mergeop.merge();
  }

  std::unique_lock<std::mutex> lk(mutex_);
  addChunk(&output_chunk, ArtifactStatus::PRESENT);

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
    logInfo("fnord.evdb", "Aborting merging for table '$0'", name_);
    // FIXPAUL delete orphaned files...
    return;
  }

  head_ = next;
  writeSnapshot();
}

void TableWriter::gc(size_t keep_generations, size_t max_generations) {
  if (keep_generations < 1) {
    RAISE(kIllegalArgumentError, "must keep at least one generation");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  auto head_gen = head_->generation;

  gcArenasWithLock();

  if (head_gen < keep_generations) {
    return;
  }

  auto first_gen = head_gen - keep_generations;

  auto cutoff = WallClock::unixMicros() - gc_delay_.microseconds();
  while (first_gen > head_gen - max_generations) {
    auto genfile = StringUtil::format(
        "$0/$1.$2.$3.idx",
        db_path_,
        name_,
        replica_id_,
        first_gen);

    if (!FileUtil::exists(genfile)) {
      return;
    }

    auto atime = FileUtil::atime(genfile);
    if (atime > cutoff / kMicrosPerSecond) {
#ifndef FNORD_NODEBUG
      logDebug(
          "fnord.evdb",
          "Skipping garbage collection for table '$0' generation $1 because " \
          "was accessed in the last $2 seconds",
          name_,
          first_gen,
          gc_delay_.seconds());
#endif
      --first_gen;
    } else {
      break;
    }
  }

  Set<String> delete_chunks;
  Set<String> delete_files;

  auto last_gen = first_gen;
  for (; last_gen > 0; --last_gen) {
    auto genfile = StringUtil::format(
        "$0/$1.$2.$3.idx",
        db_path_,
        name_,
        replica_id_,
        last_gen);

    if (!FileUtil::exists(genfile)) {
      break;
    }

    TableGeneration g;
    g.decode(FileUtil::read(genfile));

    for (const auto& c : g.chunks) {
      delete_chunks.emplace(c.replica_id + "." + c.chunk_id);
    }

    delete_files.emplace(genfile);
  }

  for (uint64_t gen = head_gen; gen > first_gen; --gen) {
    auto genfile = StringUtil::format(
      "$0/$1.$2.$3.idx",
      db_path_,
      name_,
      replica_id_,
      gen);

    TableGeneration g;
    g.decode(FileUtil::read(genfile));

    for (const auto& c : g.chunks) {
      delete_chunks.erase(c.replica_id + "." + c.chunk_id);
    }
  }

  logDebug(
      "fnord.evdb",
      "Garbage collecting table '$0'; deleting generations $1..$2, " \
      "chunks ($3): $4",
      name_,
      last_gen,
      first_gen,
      delete_chunks.size(),
      inspect(delete_chunks));

  for (const auto& c : delete_chunks) {
    auto chunkname = StringUtil::format("$0.$1", name_, c);
    auto chunkfile = StringUtil::format("$0/$1", db_path_, chunkname);

    try {
      artifacts_.deleteArtifact(chunkname);
    } catch (const Exception& e) {
      logError("fnord.evdb", e, "error while deleting artifact");
    }

    delete_files.emplace(chunkfile + ".sst");
    delete_files.emplace(chunkfile + ".cst");
    delete_files.emplace(chunkfile + ".smr");
  }

  for (const auto& f : delete_files) {
    logInfo("fnord.evdb", "Deleting file: $0", f);
    FileUtil::rm(f);
  }
}

void TableWriter::gcArenasWithLock() {
  while (arenas_.size() > 1 && arenas_.back()->isCommmited()) {
    arenas_.pop_back();
  }
}

void TableWriter::writeTable(RefPtr<TableArena> arena) {
  TableChunkRef chunk;
  chunk.replica_id = replica_id_;
  chunk.chunk_id = arena->chunkID();
  chunk.start_sequence = arena->startSequence();
  chunk.num_records = arena->records().size();

  {
    TableChunkWriter writer(db_path_, name_, &schema_, &chunk);
    for (const auto& summary_factory : summaries_) {
      writer.addSummary(summary_factory());
    }

    for (const auto& r : arena->records()) {
      writer.addRecord(r);
    }

    writer.commit();
  }

  std::unique_lock<std::mutex> lk(mutex_);

  addChunk(&chunk, ArtifactStatus::PRESENT);

  auto next = head_->clone();
  next->generation++;
  next->chunks.emplace_back(chunk);
  head_ = next;

  writeSnapshot();
  arena->commit();
  gcArenasWithLock();
  lk.unlock();
}

void TableWriter::addChunk(const TableChunkRef* chunk, ArtifactStatus status) {
  auto chunkname = StringUtil::format(
      "$0.$1.$2",
      name_,
      chunk->replica_id,
      chunk->chunk_id);

  ArtifactRef artifact;
  artifact.status = status;
  artifact.name = chunkname;

  artifact.files.emplace_back(ArtifactFileRef {
      .filename = chunkname + ".sst",
      .size = chunk->sstable_size,
      .checksum = chunk->sstable_checksum
  });

  artifact.files.emplace_back(ArtifactFileRef {
      .filename = chunkname + ".cst",
      .size = chunk->cstable_size,
      .checksum = chunk->cstable_checksum
  });

  artifact.files.emplace_back(ArtifactFileRef {
      .filename = chunkname + ".smr",
      .size = chunk->summary_size,
      .checksum = chunk->summary_checksum
  });

  artifacts_.addArtifact(artifact);
}

// precondition: mutex_ must be locked
void TableWriter::writeSnapshot() {
  auto snapname = StringUtil::format(
    "$0.$1.$2",
    name_,
    replica_id_,
    head_->generation);

  logInfo("fnord.evdb", "Writing snapshot: $0", snapname);
  auto filename = FileUtil::joinPaths(db_path_, snapname + ".idx");

  auto file = File::openFile(filename + "~", File::O_CREATE | File::O_WRITE);
  Buffer buf;
  head_->encode(&buf);
  file.write(buf);

  FileUtil::mv(filename + "~", filename);
}

const String& TableWriter::name() const {
  return name_;
}

RefPtr<TableSnapshot> TableWriter::getSnapshot() {
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
  writer.appendUInt8(0x02);
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
    writer.appendUInt64(c.summary_checksum);
    writer.appendVarUInt(c.sstable_size);
    writer.appendVarUInt(c.cstable_size);
    writer.appendVarUInt(c.summary_size);
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
    chunk.summary_checksum = *reader.readUInt64();
    if (v < 0x02) {
      chunk.sstable_size = 0;
      chunk.cstable_size = 0;
      chunk.summary_size = 0;
    } else {
      chunk.sstable_size = reader.readVarUInt();
      chunk.cstable_size = reader.readVarUInt();
      chunk.summary_size = reader.readVarUInt();
    }

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
    sstable_(sstable::SSTableEditor::create(
        chunk_filename_ + ".sst~",
        sstable::IndexProvider{},
        nullptr,
        0)),
    cstable_(new cstable::CSTableBuilder(schema)) {}

void TableChunkWriter::addSummary(RefPtr<TableChunkSummaryBuilder> summary) {
  summaries_.emplace_back(summary);
}

void TableChunkWriter::addRecord(const msg::MessageObject& record) {
  Buffer buf;
  msg::MessageEncoder::encode(record, *schema_, &buf);

  cstable_->addRecord(record);
  sstable_->appendRow(&seq_, sizeof(seq_), buf.data(), buf.size());

  for (auto& s : summaries_) {
    s->addRecord(record);
  }

  ++seq_;
}

void TableChunkWriter::commit() {
  logInfo("fnord.evdb", "Writing chunk: $0", chunk_name_);

  cstable_->write(chunk_filename_ + ".cst~");
  sstable_->finalize();

  TableChunkSummaryWriter summary_writer(chunk_filename_ + ".smr~");
  for (auto& s : summaries_) {
    s->commit(&summary_writer);
  }

  summary_writer.commit();

  chunk_->sstable_checksum = FileUtil::checksum(chunk_filename_ + ".sst~");
  chunk_->sstable_size = FileUtil::size(chunk_filename_ + ".sst~");
  chunk_->cstable_checksum = FileUtil::checksum(chunk_filename_ + ".cst~");
  chunk_->cstable_size = FileUtil::size(chunk_filename_ + ".cst~");
  chunk_->summary_checksum = FileUtil::checksum(chunk_filename_ + ".smr~");
  chunk_->summary_size = FileUtil::size(chunk_filename_ + ".smr~");

  FileUtil::mv(chunk_filename_ + ".sst~", chunk_filename_ + ".sst");
  FileUtil::mv(chunk_filename_ + ".cst~", chunk_filename_ + ".cst");
  FileUtil::mv(chunk_filename_ + ".smr~", chunk_filename_ + ".smr");
}

TableChunkMerge::TableChunkMerge(
    const String& db_path,
    const String& table_name,
    msg::MessageSchema* schema,
    const Vector<TableChunkRef> input_chunks,
    TableChunkRef* output_chunk,
    RefPtr<TableChunkWriter> writer) :
    db_path_(db_path),
    table_name_(table_name),
    schema_(schema),
    writer_(writer),
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

  writer_->commit();
}

TableMergePolicy::TableMergePolicy() {
  steps_.emplace_back(1024 * 1024 * 490, 1024 * 1024 * 520);
  //steps_.emplace_back(1024 * 1024 * 200, 1024 * 1024 * 250);
  //steps_.emplace_back(1024 * 1024 * 90, 1024 * 1024 * 100);
  steps_.emplace_back(1024 * 1024, 1024 * 1024 * 25);
}

void TableChunkMerge::readTable(const String& filename) {
  sstable::SSTableReader reader(File::openFile(filename, File::O_READ));

  if (!reader.isFinalized()) {
    RAISEF(kRuntimeError, "unfinished table chunk: $0", filename);
  }

  auto body_size = reader.bodySize();
  if (body_size == 0) {
    logWarning("fnord.evdb", "empty table chunk: $0", filename);
    return;
  }

  auto cursor = reader.getCursor();

  while (cursor->valid()) {
    auto buf = cursor->getDataBuffer();

    msg::MessageObject record;
    msg::MessageDecoder::decode(buf, *schema_, &record);

    writer_->addRecord(record);

    if (!cursor->next()) {
      break;
    }
  }
}

void TableWriter::replicateFrom(const TableGeneration& other_table) {
  std::unique_lock<std::mutex> lk(mutex_);
  bool dirty = false;

  auto next = head_->clone();
  next->generation++;
  auto& chunks = next->chunks;

  Set<String> existing_chunks_;
  for (const auto& c : chunks) {
    existing_chunks_.emplace(c.replica_id + "." + c.chunk_id);
  }

  for (const auto& c : other_table.chunks) {
    if (c.replica_id == replica_id_) {
      continue;
    }

    auto chunkname = c.replica_id + "." + c.chunk_id;
    if (existing_chunks_.count(chunkname) > 0) {
      continue;
    }

    auto cbegin = c.start_sequence;
    auto cend = c.start_sequence + c.num_records;

    bool is_subset = false;
    for (const auto& e : chunks) {
      auto ebegin = e.start_sequence;
      auto eend = e.start_sequence + e.num_records;

      if ((c.replica_id == e.replica_id) &&
          (cbegin >= ebegin) &&
          (cend <= eend)) {
        is_subset = true;
        break;
      }
    }

    if (is_subset) {
      continue;
    }

    logInfo(
        "fnord.evdb",
        "Adding foreign chunk '$0' to table '$1'",
        chunkname,
        name_);

    dirty = true;

    Set<String> dropped_chunks;
    for (auto cur = chunks.begin(); cur != chunks.end(); ) {
      auto tbegin = cur->start_sequence;
      auto tend = cur->start_sequence + cur->num_records;

      if ((cur->replica_id == c.replica_id) &&
          (tbegin >= cbegin) &&
          (tend <= cend)) {
        dropped_chunks.emplace(cur->replica_id + "." + cur->chunk_id);
        cur = chunks.erase(cur);
      } else {
        ++cur;
      }
    }

    if (dropped_chunks.size() > 0) {
        logInfo(
            "fnord.evdb",
            "Dropping foreign chunks $0 from table '$1' because they are " \
            "a strict subset of chunk '$2'",
            inspect(dropped_chunks),
            name_,
            chunkname);
    }

    addChunk(&c, ArtifactStatus::DOWNLOAD);
    chunks.emplace_back(c);
  }

  if (dirty) {
    head_ = next;
    writeSnapshot();
  }
}

bool TableMergePolicy::findNextMerge(
    RefPtr<TableSnapshot> snapshot,
    const String& db_path,
    const String& table_name,
    const String& replica_id,
    Vector<TableChunkRef>* input_chunks,
    TableChunkRef* output_chunk) {
  Vector<TableChunkRef> chunks;
  for (const auto& c : snapshot->head->chunks) {
    if (c.replica_id == replica_id) {
      chunks.emplace_back(c);
    }
  }

  if (chunks.size() < 2) {
    return false;
  }

  std::sort(chunks.begin(), chunks.end(), [] (
      const TableChunkRef& a,
      const TableChunkRef& b) -> bool {
    return a.start_sequence < b.start_sequence;
  });

  for (int i = 0; i < chunks.size() - 1; ++i) {
    for (const auto& s : steps_) {
      if (tryFoldIntoMerge(
            db_path,
            table_name,
            replica_id,
            s.first,
            s.second,
            chunks,
            i,
            input_chunks,
            output_chunk)) {
        return true;
      }
    }
  }

  return false;
}

bool TableMergePolicy::tryFoldIntoMerge(
    const String& db_path,
    const String& table_name,
    const String& replica_id,
    size_t min_merged_size,
    size_t max_merged_size,
    const Vector<TableChunkRef>& chunks,
    size_t begin,
    Vector<TableChunkRef>* input_chunks,
    TableChunkRef* output_chunk) {
  size_t cumul_size = 0;
  size_t cumul_recs = 0;

  auto end = begin;
  size_t next_seq;
  for (int i = begin; i < chunks.size(); ++i) {
    const auto& c = chunks[i];
    auto cfile = StringUtil::format(
        "$0/$1.$2.$3",
        db_path,
        table_name,
        c.replica_id,
        c.chunk_id);

    if (i > begin && c.start_sequence != next_seq) {
      logWarning(
          "fnord.evdb",
          "found record sequence discontinuity at $0 <> $1. missing chunks?",
          c.start_sequence,
          next_seq);

      break;
    }

    auto csize = FileUtil::size(cfile + ".sst");
    if (cumul_size + csize > max_merged_size) {
      break;
    }

    ++end;
    cumul_size += csize;
    cumul_recs += c.num_records;
    next_seq = c.start_sequence + c.num_records;
  }

  if (end < (begin + 2)) {
    return false;
  }

  if (cumul_size < min_merged_size) {
    return false;
  }

  for (int i = begin; i < end; ++i) {
    input_chunks->emplace_back(chunks[i]);
  }

  output_chunk->chunk_id = rnd_.hex128();
  output_chunk->start_sequence = chunks[begin].start_sequence;
  output_chunk->num_records = cumul_recs;
  output_chunk->replica_id = replica_id;
  return true;
}

void TableWriter::addSummary(SummaryFactoryFn summary) {
  summaries_.emplace_back(summary);
}

size_t TableWriter::arenaSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  size_t size = 0;

  for (const auto& a : arenas_) {
    size += a->size();
  }

  return size;
}

ArtifactIndex* TableWriter::artifactIndex() {
  return &artifacts_;
}

void TableWriter::runConsistencyCheck(
    bool check_checksums /* = false */,
    bool repair /* = false */) {
  Set<String> existing_artifacts;
  auto artifactlist = artifacts_.listArtifacts();
  for (const auto& a : artifactlist) {
    existing_artifacts.emplace(a.name);
  }

  auto snap = getSnapshot();
  for (const auto& c : snap->head->chunks) {
    auto chunkname = name_ + "." + c.replica_id + "." + c.chunk_id;
    if (existing_artifacts.count(chunkname) > 0) {
      continue;
    }

    logError(
        "fnord.evdb",
        "consistency error: chunk '$0' is missing from artifact index ",
        chunkname);

    if (repair) {
      addChunk(&c, ArtifactStatus::PRESENT);
    } else {
      RAISE(kRuntimeError, "consistency check failed");
    }
  }

  artifacts_.runConsistencyCheck(check_checksums, repair);
}

} // namespace logtable
} // namespace util

