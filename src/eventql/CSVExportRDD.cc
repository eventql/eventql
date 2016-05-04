/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/wallclock.h"
#include "eventql/util/csv/CSVOutputStream.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/CSVExportRDD.h"

using namespace stx;

namespace zbase {

CSVExportRDD::CSVExportRDD(RefPtr<VTableSource> source) : source_(source) {}

RefPtr<VFSFile> CSVExportRDD::computeBlob(dproc::TaskContext* context) {
  BufferRef buffer(new Buffer());
  CSVOutputStream csv(BufferOutputStream::fromBuffer(buffer.get()));
  csv.appendRow(source_->columns());

  source_->forEach([&csv] (const Vector<csql::SValue> row) -> bool {
    Vector<String> str_row;
    for (const auto& sv : row) {
      str_row.emplace_back(sv.getString());
    }

    csv.appendRow(str_row);
    return true;
  });

  source_->read(context);
  return buffer.get();
}

String CSVExportRDD::contentType() const {
  return "application/csv; charset=utf-8";
}

List<dproc::TaskDependency> CSVExportRDD::dependencies() const {
  return source_->dependencies();
}

}
