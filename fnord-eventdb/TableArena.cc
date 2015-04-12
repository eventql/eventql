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
    uint64_t start_sequence,
    const String& chunkid) :
    start_sequence_(start_sequence),
    chunkid_(chunkid) {}

void TableArena::addRecord(const msg::MessageObject& record) {
  records_.emplace_back(record);
}

const List<msg::MessageObject>& TableArena::records() const {
  return records_;
}

size_t TableArena::startSequence() const {
  return start_sequence_;
}

const String& TableArena::chunkID() const {
  return chunkid_;
}

} // namespace eventdb
} // namespace fnord

