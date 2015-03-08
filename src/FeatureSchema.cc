/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "fnord-base/exception.h"
#include "FeatureSchema.h"

using namespace fnord;

namespace cm {

FeatureSchema::FeatureSchema() : max_feature_id_(0) {}

Option<FeatureID> FeatureSchema::featureID(const String& feature_key) const {
  auto iter = key_to_id_index_.find(feature_key);

  if (iter == key_to_id_index_.end()) {
    return None<FeatureID>();
  } else {
    return Some(iter->second);
  }
}

Option<String> FeatureSchema::featureKey(const FeatureID& feature_id) const {
  auto iter = id_to_key_index_.find(feature_id.feature);

  if (iter == id_to_key_index_.end()) {
    return None<String>();
  } else {
    return Some(iter->second);
  }
}

const Set<uint16_t> FeatureSchema::groupIDs() const {
  return group_ids_;
}

void FeatureSchema::registerFeature(
    const String& feature_key,
    uint32_t feature_id,
    uint16_t group_id) {
  FeatureID fid = {
      .feature = feature_id,
      .group = group_id
  };

  group_ids_.emplace(group_id);
  key_to_id_index_.emplace(feature_key, fid);
  id_to_key_index_.emplace(feature_id, feature_key);

  if (feature_id > max_feature_id_) {
    max_feature_id_ = feature_id;
  }
}

void FeatureSchema::load(const Buffer& buf) {
  auto lines = StringUtil::split(buf.toString(), "\n"); // FIXPAUL slow slow slow

  for (const auto& line : lines) {
    auto parts = StringUtil::split(line, " ");

    if (parts.size() != 3) {
      RAISEF(kRuntimeError, "invalid featurefile line: $0", line);
    }

    const auto& feature_key = parts[0];
    auto feature_id = std::stoul(parts[1]);
    auto group_id = std::stoul(parts[2]);

    registerFeature(feature_key, feature_id, group_id);
  }
}

uint32_t FeatureSchema::maxFeatureID() const {
  return max_feature_id_;
}

} // namespace cm

