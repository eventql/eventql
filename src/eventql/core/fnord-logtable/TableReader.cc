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
#include <fnord-logtable/TableReader.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/util/protobuf/MessageEncoder.h>

namespace util {
namespace logtable {

static uint64_t findHeadGen(
    const String& table_name,
    const String& replica_id,
    const String& db_path) {
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

  return head_gen;
}

RefPtr<TableReader> TableReader::open(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema) {

  return new TableReader(
      table_name,
      replica_id,
      db_path,
      schema,
      findHeadGen(table_name, replica_id, db_path));
}

TableReader::TableReader(
    const String& table_name,
    const String& replica_id,
    const String& db_path,
    const msg::MessageSchema& schema,
    uint64_t head_generation) :
    name_(table_name),
    replica_id_(replica_id),
    db_path_(db_path),
    schema_(schema),
    head_gen_(head_generation) {}

const String& TableReader::name() const {
  return name_;
}

const String& TableReader::basePath() const {
  return db_path_;
}

const msg::MessageSchema& TableReader::schema() const {
  return schema_;
}

RefPtr<TableSnapshot> TableReader::getSnapshot() {
  std::unique_lock<std::mutex> lk(mutex_);
  auto g = head_gen_;
  lk.unlock();

  auto maxg = g + 100;
  while (g < maxg && !FileUtil::exists(
      StringUtil::format(
          "$0$1.$2.$3.idx",
          db_path_,
          name_,
          replica_id_,
          g))) ++g;

  if (!FileUtil::exists(
      StringUtil::format(
          "$0$1.$2.$3.idx",
          db_path_,
          name_,
          replica_id_,
          g ))) {
    g = findHeadGen(name_, replica_id_, db_path_);
  }

  while (FileUtil::exists(
      StringUtil::format(
          "$0$1.$2.$3.idx",
          db_path_,
          name_,
          replica_id_,
          g + 1))) ++g;

  if (g != head_gen_) {
    lk.lock();
    head_gen_ = g;
    lk.unlock();
  }

  RefPtr<TableGeneration> head(new TableGeneration);
  head->table_name = name_;
  head->generation = g;

  if (head_gen_ > 0) {
    auto file = FileUtil::read(StringUtil::format(
        "$0/$1.$2.$3.idx",
        db_path_,
        name_,
        replica_id_,
        g));

    head->decode(file);
  }

  return new TableSnapshot(
      head,
      List<RefPtr<logtable::TableArena>>{});
}

size_t TableReader::fetchRecords(
    const String& replica_id,
    uint64_t start_sequence,
    size_t limit,
    Function<bool (const msg::MessageObject& record)> fn) {
  auto snap = getSnapshot();

  for (const auto& c : snap->head->chunks) {
    if (c.replica_id != replica_id) {
      continue;
    }

    auto cbegin = c.start_sequence;
    auto cend = cbegin + c.num_records;

    if (cbegin <= start_sequence && cend > start_sequence) {
      auto roff = start_sequence - cbegin;
      auto rlen = c.num_records - roff;

      if (limit != size_t(-1) && rlen > limit) {
        rlen = limit;
      }

      return fetchRecords(c, roff, rlen, fn);
    }
  }

  return 0;
}

size_t TableReader::fetchRecords(
    const TableChunkRef& chunk,
    size_t offset,
    size_t limit,
    Function<bool (const msg::MessageObject& record)> fn) {
  auto filename = StringUtil::format(
      "$0/$1.$2.$3.sst",
      db_path_,
      name_,
      chunk.replica_id,
      chunk.chunk_id);

#ifndef FNORD_NOTRACE
  logTrace(
      "fnord.evdb",
      "Reading rows local=$0..$1 global=$2..$3 from table=$4 chunk=$5",
      offset,
      offset + limit,
      chunk.start_sequence + offset,
      chunk.start_sequence + offset + limit,
      name_,
      chunk.replica_id + "." + chunk.chunk_id);
#endif

  sstable::SSTableReader reader(File::openFile(filename, File::O_READ));

  if (!reader.isFinalized()) {
    RAISEF(kRuntimeError, "unfinished table chunk: $0", filename);
  }

  auto body_size = reader.bodySize();
  if (body_size == 0) {
    logWarning("fnord.evdb", "empty table chunk: $0", filename);
    return 0;
  }

  size_t n = 0;
  auto cursor = reader.getCursor();
  while (cursor->valid()) {
    ++n;

    if (n > offset) {
      auto buf = cursor->getDataBuffer();
      msg::MessageObject record;
      msg::MessageDecoder::decode(buf, schema_, &record);
      fn(record);
    }

    if (n == limit + offset) {
      break;
    }

    if (!cursor->next()) {
      break;
    }
  }

  return n;
}

} // namespace logtable
} // namespace util

