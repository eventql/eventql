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
#include <fnord-base/logging.h>
#include <fnord-tsdb/CSTableIndex.h>
#include <fnord-tsdb/RecordSet.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-cstable/CSTableBuilder.h>

namespace fnord {
namespace tsdb {

CSTableIndex::CSTableIndex(
    RefPtr<msg::MessageSchema> schema) :
    schema_(schema) {}

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
  auto filename = records->filenamePrefix() + rnd_.hex64() + ".cst";

  fnord::logDebug(
      "fnord.tsdb",
      "Building cstable: $0",
      filename);

  cstable::CSTableBuilder cstable(schema_.get());
  auto cur = records->firstOffset();
  auto end = records->lastOffset();
  while (cur < end) {
    records->fetchRecords(cur, 0xffffffff, [this, &cstable, &cur] (
        uint64_t msgid,
        const void* data,
        size_t size) {
      ++cur;
      msg::MessageObject obj;
      msg::MessageDecoder::decode(data, size, *schema_, &obj);
      cstable.addRecord(obj);
    });
  }

  cstable.write(filename);
  new_state->append(filename.data(), filename.size());

  if (last_state.size() > 0) {
    delete_after_commit->emplace(last_state.toString());
  }
}

}
}
