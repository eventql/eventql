/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/MessageDecoder.h>
#include <zbase/core/StaticPartitionReader.h>
#include <zbase/core/Table.h>
#include <cstable/CSTableReader.h>
#include <cstable/RecordMaterializer.h>
#include <csql/runtime/EmptyTable.h>

using namespace stx;

namespace zbase {

StaticPartitionReader::StaticPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

void StaticPartitionReader::fetchRecords(
    Function<void (const msg::MessageObject& record)> fn) {
  auto schema = table_->schema();

  auto cstable = fetchCSTableFilename();
  if (cstable.isEmpty()) {
    return;
  }

  auto reader = cstable::CSTableReader::openFile(cstable.get());
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
  auto cstable_version = cstableVersion();
  if (cstable_version.isEmpty()) {
    return SHA1Hash{};
  } else {
    return cstable_version.get();
  }
}

ScopedPtr<csql::TableExpression> StaticPartitionReader::buildSQLScan(
    csql::Transaction* ctx,
    RefPtr<csql::SequentialScanNode> node,
    csql::QueryBuilder* runtime) const {
  auto cstable = fetchCSTableFilename();
  if (cstable.isEmpty()) {
    return mkScoped(new csql::EmptyTable(node->outputColumns()));
  }

  auto scan = mkScoped(
      new csql::CSTableScan(
          ctx,
          node,
          cstable.get(),
          runtime));

  auto cstable_version = cstableVersion();
  if (!cstable_version.isEmpty()) {
    scan->setCacheKey(cstable_version.get());
  }

  return std::move(scan);
}

} // namespace tdsb

