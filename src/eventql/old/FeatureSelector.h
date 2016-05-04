/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATURESELECTOR_H
#define _CM_FEATURESELECTOR_H
#include "eventql/util/stdtypes.h"
#include "eventql/util/option.h"
#include "FeatureID.h"
#
#include <eventql/util/fts.h>
#include <eventql/util/fts_common.h>

using namespace stx;

namespace zbase {

class FeatureSelector {
public:

  FeatureSelector(FeatureIndex* feature_index, stx::fts::Analyzer* analyzer);

  void featuresFor(
      const JoinedQuery& query,
      const JoinedQueryItem& item,
      Set<String>* features);

protected:
  FeatureIndex* feature_index_;
  stx::fts::Analyzer* analyzer_;
};

} // namespace zbase

#endif
