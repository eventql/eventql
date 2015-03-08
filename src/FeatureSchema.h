/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATURESCHEMA_H
#define _CM_FEATURESCHEMA_H
#include "fnord-base/stdtypes.h"
#include "fnord-base/option.h"
#include "FeatureID.h"

using namespace fnord;

namespace cm {

class FeatureSchema {
public:

  FeatureSchema();

  Option<FeatureID> featureID(const String& feature_key) const;
  Option<String> featureKey(const FeatureID& feature_id) const;

  const Set<uint16_t> groupIDs() const;

  void registerFeature(
      const String& feature_key,
      uint32_t feature_id,
      uint16_t group_id);

  void load(const Buffer& buf);

  uint32_t maxFeatureID() const;

protected:
  Set<uint16_t> group_ids_;
  HashMap<String, FeatureID> key_to_id_index_;
  HashMap<uint32_t, String> id_to_key_index_;
  uint32_t max_feature_id_;
};

} // namespace cm

#endif
