/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/stdtypes.h>
#include <fnord-tsdb/CSTableIndex.h>

namespace fnord {
namespace tsdb {

String CSTableIndex::name() {
  return "cstable";
}

void CSTableIndex::update(
    RecordSet* records,
    uint64_t last_offset,
    uint64_t cur_offset,
    const Buffer& last_state,
    Buffer* new_state,
    Set<String>* delete_after_commit) {
  fnord::iputs("update cstable: $0 .. $1", last_offset, cur_offset);
}

}
}
