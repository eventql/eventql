/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/RemoteTableReader.h>

namespace fnord {
namespace logtable {

RemoteTableReader::RemoteTableReader(
    const String& table_name,
    const msg::MessageSchema& schema,
    const URI& uri,
    http::HTTPConnectionPool* http) :
    name_(table_name),
    schema_(schema),
    uri_(uri),
    http_(http) {}

const String& RemoteTableReader::name() const {
  return name_;
}

const msg::MessageSchema& RemoteTableReader::schema() const {
  return schema_;
}

//RefPtr<TableSnapshot> RemoteTableReader::getSnapshot() {
//}

//size_t RemoteTableReader::fetchRecords(
//    const String& replica,
//    uint64_t start_sequence,
//    size_t limit,
//    Function<bool (const msg::MessageObject& record)> fn) {
//}

} // namespace logtable
} // namespace fnord


