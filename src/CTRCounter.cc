/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include "CTRCounter.h"

using namespace fnord;

namespace cm {

sstable::SSTableColumnSchema CTRCounterData::sstableSchema() {
  sstable::SSTableColumnSchema schema;
  schema.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
  return schema;
}

CTRCounterData CTRCounterData::load(const String& str) {
  static auto schema = sstableSchema();

  CTRCounterData c;

  Buffer buf(str.data(), str.size());
  sstable::SSTableColumnReader cols(&schema, buf);
  c.num_views = cols.getUInt64Column(schema.columnID("num_views"));
  c.num_clicks = cols.getUInt64Column(schema.columnID("num_clicks"));
  c.num_clicked = cols.getUInt64Column(schema.columnID("num_clicked"));

  return c;
}

CTRCounterData::CTRCounterData() :
    num_views(0),
    num_clicks(0),
    num_clicked(0) {}

void CTRCounterData::merge(const CTRCounterData& other) {
  num_views += other.num_views;
  num_clicks += other.num_clicks;
  num_clicked += other.num_clicked;
}

void CTRCounterData::encode(util::BinaryMessageWriter* writer) const {
  writer->appendUInt64(num_views);
  writer->appendUInt64(num_clicks);
  writer->appendUInt64(num_clicked);
}

void CTRCounterData::decode(util::BinaryMessageReader* reader) {
  num_views = *reader->readUInt64();
  num_clicks = *reader->readUInt64();
  num_clicked = *reader->readUInt64();
}

}
