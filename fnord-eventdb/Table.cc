/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-eventdb/Table.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-msg/MessagePrinter.h>

namespace fnord {
namespace eventdb {

Table::Table(
    const String& table_name,
    const String& replica_id,
    const msg::MessageSchema& schema) :
    name_(table_name),
    replica_id_(replica_id),
    schema_(schema),
    seq_(1) {
  arenas_.emplace_front(new TableArena(seq_));
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
  const auto& records = arenas_.front()->records();

  if (records.size() == 0) {
    return 0;
  }

  arenas_.emplace_front(new TableArena(seq_));
  return records.size();
}

const String& Table::name() const {
  return name_;
}

} // namespace eventdb
} // namespace fnord

