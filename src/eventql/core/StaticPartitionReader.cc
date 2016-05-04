/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/fnv.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/core/StaticPartitionReader.h>
#include <eventql/core/Table.h>
#include <eventql/infra/cstable/CSTableReader.h>
#include <eventql/infra/cstable/RecordMaterializer.h>

using namespace stx;

namespace zbase {

StaticPartitionReader::StaticPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

void StaticPartitionReader::fetchRecords(
    const Set<String>& required_columns,
    Function<void (const msg::MessageObject& record)> fn) {
  auto schema = table_->schema();

  auto cstable = FileUtil::joinPaths(snap_->base_path, "_cstable");
  if (!FileUtil::exists(cstable)) {
    return;
  }

  auto reader = cstable::CSTableReader::openFile(cstable);
  cstable::RecordMaterializer materializer(
      schema.get(),
      reader.get());

  auto rec_count = reader->numRecords();
  for (size_t i = 0; i < rec_count; ++i) {
    msg::MessageObject robj;
    materializer.nextRecord(&robj);
    fn(robj);
  }
}

SHA1Hash StaticPartitionReader::version() const {
  auto metapath = FileUtil::joinPaths(snap_->base_path, "_cstable_state");

  if (FileUtil::exists(metapath)) {
    return SHA1::compute(FileUtil::read(metapath));
  }

  if (snap_->state.has_cstable_version()) {
    return SHA1::compute(StringUtil::toString(snap_->state.cstable_version()));
  }

  return SHA1Hash{};
}

} // namespace tdsb

