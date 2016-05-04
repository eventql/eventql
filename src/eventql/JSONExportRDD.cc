
/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/json/json.h"
#include "stx/protobuf/msg.h"
#include "eventql/JSONExportRDD.h"

using namespace stx;

namespace zbase {

JSONExportRDD::JSONExportRDD(RefPtr<VTableSource> source) : source_(source) {}

RefPtr<VFSFile> JSONExportRDD::computeBlob(dproc::TaskContext* context) {
  auto cols = source_->columns();

  BufferRef buffer(new Buffer());
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(buffer.get()));
  json.beginObject();

  json.addObjectEntry("columns");
  json::toJSON(cols, &json);
  json.addComma();

  json.addObjectEntry("rows");
  json.beginArray();

  size_t n = 0;
  source_->forEach([&json, &cols, &n] (const Vector<csql::SValue> row) -> bool {
    if (++n > 1) {
      json.addComma();
    }

    json.beginObject();
    for (int i = 0; i < std::min(cols.size(), row.size()); ++i) {
      if (i > 0) {
        json.addComma();
      }

      json.addObjectEntry(cols[i]);

      switch (row[i].getType()) {
        case SQL_FLOAT:
          json.addFloat(row[i].getFloat());
          break;

        case SQL_INTEGER:
          json.addInteger(row[i].getInteger());
          break;

        case SQL_BOOL:
          row[i].getBool() ? json.addTrue() : json.addFalse();
          break;

        default:
          json.addString(row[i].getString());
          break;
      }
    }

    json.endObject();

    return true;
  });

  source_->read(context);

  json.endArray();
  json.endObject();

  return buffer.get();
}

String JSONExportRDD::contentType() const {
  return "application/json; charset=utf-8";
}

List<dproc::TaskDependency> JSONExportRDD::dependencies() const {
  return source_->dependencies();
}

}
