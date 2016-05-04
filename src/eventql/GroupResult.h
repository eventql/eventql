/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_GROURESULT_H
#define _CM_GROURESULT_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/json/json.h>
#include "eventql/infra/cstable/CSTableReader.h"
#include "eventql/CTRCounter.h"
#include "eventql/AnalyticsTableScan.h"
#include "eventql/TrafficSegment.h"
#include "eventql/AnalyticsQueryResult.h"

using namespace stx;

namespace zbase {

template <typename GroupKeyType, typename GroupValueType>
struct GroupResult : public AnalyticsQueryResult::SubQueryResult {
  void merge(const AnalyticsQueryResult::SubQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const override;
  void encode(util::BinaryMessageWriter* writer) const override;
  void decode(util::BinaryMessageReader* reader) override;

  Vector<HashMap<GroupKeyType, GroupValueType>> counters;
  Vector<String> segment_keys;
};

} // namespace zbase

#include "GroupResult_impl.h"
#endif
