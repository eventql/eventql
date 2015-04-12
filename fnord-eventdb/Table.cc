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
    const msg::MessageSchema& schema) :
    name_(table_name),
    schema_(schema) {}

void Table::addRecords(const Buffer& records) {

  int n = 0;
  for (size_t offset = 0; offset < records.size(); ++n) {
    msg::MessageObject msg;
    msg::MessageDecoder::decode(records, schema_, &msg, &offset);
    addRecord(msg);
  }
}

void Table::addRecord(const msg::MessageObject& record) {
  fnord::iputs("add_record: $0", msg::MessagePrinter::print(record, schema_));
}

const String& Table::name() const {
  return name_;
}

} // namespace eventdb
} // namespace fnord

