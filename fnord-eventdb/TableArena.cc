/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-eventdb/TableArena.h>

namespace fnord {
namespace eventdb {

TableArena::TableArena(
    uint64_t start_offset) :
    start_offset_(start_offset) {}

void TableArena::addRecord(const msg::MessageObject& record) {
  records_.emplace_back(record);
}

const List<msg::MessageObject>& TableArena::records() const {
  return records_;
}

} // namespace eventdb
} // namespace fnord

