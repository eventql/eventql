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

csql::TaskIDList StaticPartitionReader::buildSQLScan(
    csql::Transaction* txn,
    RefPtr<csql::SequentialScanNode> seqscan,
    csql::TaskDAG* tasks) const {
  auto cstable = fetchCSTableFilename();
  if (cstable.isEmpty()) {
    return csql::TaskIDList{};
  }

  // auto cstable_version = cstableVersion();
  // if (!cstable_version.isEmpty()) {
  //   scan->setCacheKey(cstable_version.get());
  // }

  auto task_factory = [seqscan, cstable] (
      csql::Transaction* txn,
      csql::RowSinkFn output) -> RefPtr<csql::Task> {
    return new csql::CSTableScan(
        txn,
        seqscan,
        cstable.get(),
        txn->getRuntime()->queryBuilder().get(),
        output);
  };

  auto task = new csql::TaskDAGNode(
      new csql::SimpleTableExpressionFactory(task_factory));

  csql::TaskIDList output;
  output.emplace_back(tasks->addTask(task));
  return output;
}

} // namespace tdsb

