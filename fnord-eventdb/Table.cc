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

namespace fnord {
namespace eventdb {

Table::Table(
    const String& table_name,
    const msg::MessageSchema& schema) :
    name_(table_name),
    schema_(schema) {}

void Table::addRecords(const Buffer& records) {
}

void Table::addRecord(const msg::MessageObject& record) {
}

const String& Table::name() const {
  return name_;
}

} // namespace eventdb
} // namespace fnord

