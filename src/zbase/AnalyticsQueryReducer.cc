/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/AnalyticsApp.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/AnalyticsQuery.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/AnalyticsQueryMapper.h"
#include <zbase/core/TimeWindowPartitioner.h>

using namespace stx;

namespace zbase {

AnalyticsQueryReducer::AnalyticsQueryReducer(
      const AnalyticsQuerySpec& spec,
      zbase::TSDBService* tsdb,
      AnalyticsQueryFactory* factory) :
      spec_(spec),
      tsdb_(tsdb),
      factory_(factory),
      result_(nullptr) {
  query_.build(spec_);
  result_ = factory_->buildQuery(query_, &scan_);
}

List<dproc::TaskDependency> AnalyticsQueryReducer::dependencies() const {
  auto shards = AnalyticsTableScanPlanner::mapShards(
      spec_.customer(),
      spec_.start_time(),
      spec_.end_time(),
      "AnalyticsQueryMapper",
      *msg::encode(spec_),
      tsdb_);

  for (auto cur = shards.begin(); cur != shards.end(); ) {
    auto p = msg::decode<zbase::TSDBTableScanSpec>(cur->params);

    if (p.use_cstable_index()) {
      ++cur;
    } else {
      // we do not support non cstable indexed partitions
      cur = shards.erase(cur);
    }
  }

  return shards;
}

void AnalyticsQueryReducer::compute(dproc::TaskContext* context) {
  for (int i = 0; i < context->numDependencies(); ++i) {
    auto shard = context
        ->getDependency(i)
        ->getInstanceAs<AnalyticsQueryMapper>();

    result_->merge(*shard->queryResult());
  }

  for (int i = 0; i < result_->results.size(); ++i) {
    auto& r = result_->results[i];
    result_->subqueries[i]->reduceResult(r);
    r->applyTimeRange(result_->query.start_time, result_->query.end_time);
  }
}

RefPtr<VFSFile> AnalyticsQueryReducer::encode() const {
  util::BinaryMessageWriter buf;
  result_->encode(&buf);
  return new Buffer(buf.data(), buf.size());
}

void AnalyticsQueryReducer::decode(RefPtr<VFSFile> data) {
  util::BinaryMessageReader buf(data->data(), data->size());
  result_->decode(&buf);
}

void AnalyticsQueryReducer::toJSON(json::JSONOutputStream* json) {
  result_->toJSON(json);
}

void AnalyticsQueryReducer::toCSV(CSVOutputStream* csv) {
  result_->toCSV(csv);
}

RefPtr<AnalyticsQueryResult> AnalyticsQueryReducer::queryResult() {
  return result_;
}

//BufferRef buf(new Buffer());

  /* format: csv */
  //if (format == "csv") {
  //  Set<String> columns;
  //  //for (auto p : params) {
  //  //  if (p.first == "column") {
  //  //    columns.insert(p.second);
  //  //  }
  //  //}

  //  for (const auto& subq_result : result.results) {
  //    subq_result->toCSV(buf.get(), columns);
  //  }
  //}

  /* format: json */
  //if (format == "json") {
//    json::JSONOutputStream json(BufferOutputStream::fromBuffer(buf.get()));
//    
//  //}
//
//  return buf.get();

} // namespace zbase

