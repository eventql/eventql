/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/util/fnv.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/core/StaticPartitionReader.h>
#include <eventql/core/Table.h>
#include <eventql/io/cstable/CSTableReader.h>
#include <eventql/io/cstable/RecordMaterializer.h>

#include "eventql/eventql.h"

namespace eventql {

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

