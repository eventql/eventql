/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/AnalyticsQueryResult.h"

using namespace stx;

namespace zbase {

AnalyticsQueryResult::AnalyticsQueryResult(
    const AnalyticsQuery& q) :
    query(q),
    execution_time(0),
    rows_scanned(0),
    rows_missing(0) {}

void AnalyticsQueryResult::merge(const AnalyticsQueryResult& other) {
  rows_scanned += other.rows_scanned;
  rows_missing += other.rows_missing;
  error += other.error;

  for (int i = 0; i < results.size() && i < other.results.size(); ++i) {
    results[i]->merge(*other.results[i]);
  }

  for (int i = results.size(); i < other.results.size(); ++i) {
    results.emplace_back(other.results[i]);
    subqueries.emplace_back(other.subqueries[i]);
  }
}

void AnalyticsQueryResult::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("rows_scanned");
  json->addInteger(rows_scanned);
  json->addComma();
  json->addObjectEntry("rows_missing");
  json->addInteger(rows_missing);
  json->addComma();
  json->addObjectEntry("execution_time_ms");
  json->addFloat(execution_time / 1000.0f);
  json->addComma();
  json->addObjectEntry("query");
  query.toJSON(json);

  if (error.length() > 0) {
    json->addComma();
    json->addObjectEntry("error");
    json->addString(error);
  } else {
    json->addComma();
    json->addObjectEntry("results");
    json->beginObject();

    for (int i = 0; i < results.size(); ++i) {
      if (i > 0) json->addComma();
      json->addObjectEntry(query.queries[i].query_type);
      results[i]->toJSON(json);
    }

    json->endObject();
  }

  json->endObject();
}

void AnalyticsQueryResult::toCSV(CSVOutputStream* csv) const {
  for (int i = 0; i < results.size(); ++i) {
    results[i]->toCSV(csv);
  }
}

void AnalyticsQueryResult::encode(util::BinaryMessageWriter* writer) const {
  writer->appendUInt64(rows_scanned);
  writer->appendUInt32(results.size());

  for (const auto& r : results) {
    r->encode(writer);
  }
}

void AnalyticsQueryResult::decode(util::BinaryMessageReader* reader) {
  rows_scanned = *reader->readUInt64();
  auto num_res = *reader->readUInt32();

  for (const auto& r : results) {
    r->decode(reader);
  }
}

} // namespace zbase

