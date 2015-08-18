/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRAFFISEGMENT_H
#define _CM_TRAFFISEGMENT_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/json/json.h>
#include "cstable/CSTableReader.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"

using namespace stx;

namespace zbase {

enum class TrafficSegmentOp {
  MATCHES,
  MATCHES_UINT32,
  INCLUDESI
};

TrafficSegmentOp trafficSegmentOpFromString(String op);
String trafficSegmentOpToString(TrafficSegmentOp op);

typedef Tuple<String, TrafficSegmentOp, Vector<String>> TrafficSegmentRule;

struct TrafficSegmentParams {
  String key;
  String name;
  Vector<TrafficSegmentRule> rules;
  String color;

  void toJSON(json::JSONOutputStream* json) const;
};

class TrafficSegment : public RefCounted {
public:

  TrafficSegment(
      const TrafficSegmentParams& params,
      AnalyticsTableScan* scan);

  const String& key() const;
  bool checkPredicates();

protected:

  void addRule(
      const TrafficSegmentRule& ruleo,
      AnalyticsTableScan* scan);

  TrafficSegmentParams params_;
  List<Function<bool()>> predicates_;
};

} // namespace zbase

#endif
