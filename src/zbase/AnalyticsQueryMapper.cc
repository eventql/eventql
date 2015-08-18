/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "stx/fnv.h"
#include "cstable/CSTableReader.h"
#include <tsdb/TSDBTableScanSpec.pb.h>
#include <tsdb/CSTableIndex.h>
#include "zbase/AnalyticsApp.h"
#include "zbase/AnalyticsQueryMapper.h"
#include "zbase/AnalyticsQuery.h"
#include "zbase/AnalyticsQueryResult.h"

using namespace stx;

namespace cm {

AnalyticsQueryMapper::AnalyticsQueryMapper(
      const tsdb::TSDBTableScanSpec& params,
      tsdb::PartitionMap* pmap,
      AnalyticsQueryFactory* factory) :
      params_(params),
      pmap_(pmap),
      factory_(factory),
      result_(nullptr) {
  msg::decode(
      params_.scanlet_params().data(),
      params_.scanlet_params().size(),
      &spec_);

  query_.build(spec_);
  query_.rewrite();

  result_ = factory_->buildQuery(query_, &scan_);
}

//List<dproc::TaskDependency> AnalyticsQueryMapper::dependencies() const {
  //List<dproc::TaskDependency> deps;

  //tsdb::TSDBTableScanSpec dparams;
  //dparams.set_schema_name("cm.JoinedSession");
  //dparams.set_version(params_.version());

  //deps.emplace_back(dproc::TaskDependency {
  //  .task_name = "CSTableIndex",
  //  .params = *msg::encode(dparams)
  //});

  //return deps;
//}

RefPtr<VFSFile> AnalyticsQueryMapper::encode() const {
  util::BinaryMessageWriter buf;
  result_->encode(&buf);
  return new Buffer(buf.data(), buf.size());
}

void AnalyticsQueryMapper::decode(RefPtr<VFSFile> data) {
  util::BinaryMessageReader buf(data->data(), data->size());
  result_->decode(&buf);
}

void AnalyticsQueryMapper::compute(dproc::TaskContext* context) {
  try {
    auto partition = pmap_->findPartition(
        spec_.customer(),
        params_.table_name(),
        SHA1Hash::fromHexString(params_.partition_key()));

    if (partition.isEmpty()) {
      return;
    }

    auto preader = partition.get()->getReader();
    auto cstable = preader->fetchCSTableFilename();
    if (cstable.isEmpty()) {
      return;
    }

    cstable::CSTableReader reader(cstable.get());
    scan_.scanTable(&reader);
    result_->rows_scanned = scan_.rowsScanned();
  } catch (const std::exception& e) {
    stx::logError(
        "cm.analyticsqueryengine",
        e,
        "error while reading partition: '$0'",
        params_.partition_key());

    result_->error += e.what();
    result_->error += " while reading partition: " + params_.partition_key();
  }
}

Option<String> AnalyticsQueryMapper::cacheKey() const {
  FNV<uint64_t> fnv;
  uint64_t h[2];

  auto key = params_.partition_key();
  key += StringUtil::format("~$0", params_.version());

  for (const auto& subq : result_->subqueries) {
    key += StringUtil::format("~$0", subq->version());
  }

  h[0] = fnv.hash(key);
  for (const auto& q : query_.queries) {
    key += "~";
    key += q.query_type;
    for (const auto& p : q.params) {
      if (p.first == "limit" ||
          p.first == "offset" ||
          p.first == "order_by" ||
          p.first == "order_fn") {
        continue;
      }

      key += "~" + p.first;
      key += "~" + p.second;
    }
  }

  json::JSONOutputStream json(StringOutputStream::fromString(&key));
  for (const auto& s : query_.segments) {
    key += "~";
    s.toJSON(&json);
  }

  h[1] = fnv.hash(key);

  return Some(
      "cm.analytics.qrycache~" + 
      StringUtil::hexPrint(h, sizeof(uint64_t) * 2, false));
}

RefPtr<AnalyticsQueryResult> AnalyticsQueryMapper::queryResult() const {
  return result_;
}

} // namespace cm

