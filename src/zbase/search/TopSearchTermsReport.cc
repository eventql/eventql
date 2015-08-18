/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/protobuf/msg.h"
#include "stx/util/Base64.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/AnalyticsQueryParams.pb.h"
#include "TopSearchTermsReport.h"

using namespace stx;

namespace cm {

TopSearchTermsReport::TopSearchTermsReport(
    const ReportParams& params,
    const Option<String>& segments) :
    params_(params),
    segments_(segments) {}

RefPtr<VFSFile> TopSearchTermsReport::computeBlob(dproc::TaskContext* context) {
  auto dep = context->getDependency(0)->getInstanceAs<AnalyticsQueryReducer>();
  auto res = dep->queryResult();

  BufferRef buffer(new Buffer());
  CSVOutputStream csv(BufferOutputStream::fromBuffer(buffer.get()));
  res->results[0]->toCSV(&csv);

  return buffer.get();
}

String TopSearchTermsReport::contentType() const {
  return "application/csv; charset=utf-8";
}

List<dproc::TaskDependency> TopSearchTermsReport::dependencies() const {
  AnalyticsQuerySpec query;
  query.set_customer(params_.customer());
  query.set_start_time(params_.from_unixmicros());
  query.set_end_time(params_.until_unixmicros());

  if (!segments_.isEmpty()) {
    query.set_segments(segments_.get());
  }

  auto subquery = query.add_subqueries();
  subquery->set_query("search.TopSearchTermsQuery");
  {
    auto p = subquery->add_params();
    p->set_key("limit");
    p->set_value("1000000");
  }

  return List<dproc::TaskDependency> {
    dproc::TaskDependency {
      .task_name = "AnalyticsQueryReducer",
      .params = *msg::encode(query)
    }
  };
}

}
