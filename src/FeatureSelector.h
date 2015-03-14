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
#include "fnord-base/stdtypes.h"
#include "fnord-base/option.h"
#include "FeatureID.h"
#include "JoinedQuery.h"

using namespace fnord;

namespace cm {

class FeatureSelector {
public:

  FeatureSelector(FeatureIndex* feature_index);

  void featuresFor(
      const JoinedQuery& query,
      const JoinedQueryItem& item,
      Set<String>* features);

protected:
  FeatureIndex* feature_index_;
};

} // namespace cm

#endif
