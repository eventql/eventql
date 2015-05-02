/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-tsdb/RecordSet.h>

namespace fnord {
namespace tsdb {

RecordSet::RecordSet(
    RefPtr<msg::MessageSchema> schema,
    const String& filename_prefix) :
    schema_(schema),
    filename_prefix_(filename_prefix) {}

RecordSet::RecordSetState RecordSet::getState() {
  return state_;
}

void RecordSet::addRecord(uint32_t record_id, const Buffer& message) {
  
}

RecordSet::RecordSetState::RecordSetState() : commitlog_size(0) {}

} // namespace tsdb
} // namespace fnord

