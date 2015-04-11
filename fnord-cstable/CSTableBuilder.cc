/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/CSTableBuilder.h>

namespace fnord {
namespace cstable {

CSTableBuilder::CSTableBuilder(
    msg::MessageSchema* schema,
    HashMap<String, RefPtr<ColumnWriter>> column_writers) :
    schema_(schema),
    columns_(column_writers) {}

void CSTableBuilder::addRecord(const Buffer& buf) {
}

void CSTableBuilder::write(const String& filename) {
}


} // namespace cstable
} // namespace fnord

