/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "RecoStats.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

RecoStats::RecoStats() :
    num_queries(0),
    num_queries_with_recos(0),
    num_queries_with_recos_seen(0),
    num_queries_with_recos_clicked(0),
    num_reco_items(0),
    num_reco_items_seen(0),
    num_reco_items_clicked(0) {}

void RecoStats::merge(const RecoStats& other) {
  num_queries += other.num_queries;
  num_queries_with_recos += other.num_queries_with_recos;
  num_queries_with_recos_seen += other.num_queries_with_recos_seen;
  num_queries_with_recos_clicked += other.num_queries_with_recos_clicked;
  num_reco_items += other.num_reco_items;
  num_reco_items_seen += other.num_reco_items_seen;
  num_reco_items_clicked += other.num_reco_items_clicked;
}

void RecoStats::encode(util::BinaryMessageWriter* writer) const {
  writer->appendUInt8(0x01);
  writer->appendVarUInt(num_queries);
  writer->appendVarUInt(num_queries_with_recos);
  writer->appendVarUInt(num_queries_with_recos_seen);
  writer->appendVarUInt(num_queries_with_recos_clicked);
  writer->appendVarUInt(num_reco_items);
  writer->appendVarUInt(num_reco_items_seen);
  writer->appendVarUInt(num_reco_items_clicked);
}

void RecoStats::decode(util::BinaryMessageReader* reader) {
  if (*reader->readUInt8() != 0x01) {
    RAISE(kRuntimeError, "unsupported version");
  }

  num_queries = reader->readVarUInt();
  num_queries_with_recos = reader->readVarUInt();
  num_queries_with_recos_seen = reader->readVarUInt();
  num_queries_with_recos_clicked = reader->readVarUInt();
  num_reco_items = reader->readVarUInt();
  num_reco_items_seen = reader->readVarUInt();
  num_reco_items_clicked = reader->readVarUInt();
}

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

void RecoStats::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();

  json->addObjectEntry("num_queries");
  json->addInteger(num_queries);
  json->addComma();

  json->addObjectEntry("num_queries_with_recos");
  json->addInteger(num_queries_with_recos);
  json->addComma();

  json->addObjectEntry("num_queries_with_recos_seen");
  json->addInteger(num_queries_with_recos_seen);
  json->addComma();

  json->addObjectEntry("reco_available_rate");
  json->addFloat(mkRate(num_queries_with_recos, num_queries));
  json->addComma();

  json->addObjectEntry("reco_seen_rate");
  json->addFloat(mkRate(num_queries_with_recos_seen, num_queries));
  json->addComma();

  json->addObjectEntry("reco_ctr");
  json->addFloat(mkRate(num_queries_with_recos_clicked, num_queries_with_recos));
  json->addComma();

  json->addObjectEntry("reco_seen_ctr");
  json->addFloat(mkRate(num_queries_with_recos_clicked, num_queries_with_recos_seen));
  json->addComma();

  json->addObjectEntry("reco_item_ctr");
  json->addFloat(mkRate(num_reco_items_clicked, num_reco_items));
  json->addComma();

  json->addObjectEntry("reco_item_seen_ctr");
  json->addFloat(mkRate(num_reco_items_clicked, num_reco_items_seen));

  json->endObject();
}


} // namespace zbase

