/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include "zbase/CTRCounter.h"

using namespace stx;

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

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

CTRCounterData::CTRCounterData() :
    num_views(0),
    num_clicks(0),
    num_clicked(0),
    gmv_eurcent(0),
    cart_value_eurcent(0) {}

void CTRCounterData::merge(const CTRCounterData& other) {
  num_views += other.num_views;
  num_clicks += other.num_clicks;
  num_clicked += other.num_clicked;
  gmv_eurcent += other.gmv_eurcent;
  cart_value_eurcent += other.cart_value_eurcent;
}

void CTRCounterData::encode(util::BinaryMessageWriter* writer) const {
  writer->appendVarUInt(num_views);
  writer->appendVarUInt(num_clicks);
  writer->appendVarUInt(num_clicked);
  writer->appendVarUInt(gmv_eurcent);
  writer->appendVarUInt(cart_value_eurcent);
}

void CTRCounterData::decode(util::BinaryMessageReader* reader) {
  num_views = reader->readVarUInt();
  num_clicks = reader->readVarUInt();
  num_clicked = reader->readVarUInt();
  gmv_eurcent = reader->readVarUInt();
  cart_value_eurcent = reader->readVarUInt();
}

void CTRCounterData::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("views");
  json->addInteger(num_views);
  json->addComma();
  json->addObjectEntry("clicks");
  json->addInteger(num_clicks);
  json->addComma();
  json->addObjectEntry("clicked");
  json->addInteger(num_clicked);
  json->addComma();
  json->addObjectEntry("ctr");
  json->addFloat(num_clicks / (double) num_views);
  json->addComma();
  json->addObjectEntry("cpq");
  json->addFloat(num_clicked / (double) num_views);
  json->addComma();
  json->addObjectEntry("total_gmv_eurcent");
  json->addInteger(gmv_eurcent);
  json->addComma();
  json->addObjectEntry("total_cart_value_eurcent");
  json->addInteger(cart_value_eurcent);
  json->addComma();
  json->addObjectEntry("avg_gmv_eurcent");
  json->addInteger(mkRate(gmv_eurcent, num_views));
  json->addComma();
  json->addObjectEntry("avg_cart_value_eurcent");
  json->addInteger(mkRate(cart_value_eurcent, num_views));
  json->endObject();
}

}
