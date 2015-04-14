/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <thread>
#include <fnord-eventdb/TableReader.h>
#include <fnord-base/logging.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-msg/MessageEncoder.h>

namespace fnord {
namespace eventdb {

RefPtr<TableReader> TableReader::open(
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

  return new TableReader(
      table_name,
      replica_id,
      db_path,
      schema,
      head_gen);
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

RefPtr<TableSnapshot> TableReader::getSnapshot() {
  std::unique_lock<std::mutex> lk(mutex_);
  auto g = head_gen_;
  lk.unlock();

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
      List<RefPtr<fnord::eventdb::TableArena>>{});
}

} // namespace eventdb
} // namespace fnord

