/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <fnord-base/inspect.h>
#include "FeaturePack.h"

namespace cm {

FeaturePackReader::FeaturePackReader(
    const void* data,
    size_t size) :
    fnord::util::BinaryMessageReader(data, size) {}

void FeaturePackReader::readFeatures(FeaturePack* pack) {
  auto num_features = *readUInt32();

  Vector<Pair<uint32_t, uint32_t>> hdr;
  for (int i = 0; i < num_features; ++i) {
    auto feature = *readUInt32();
    auto offset = *readUInt32();
    hdr.emplace_back(feature, offset);
  }

  for (int i = 0; i < hdr.size(); ++i) {
    auto size = (i == (hdr.size() - 1))
        ? size_ - hdr[i].second
        : hdr[i + 1].second - hdr[i].second;

    auto data = read(size);
    FeatureID fid = { .feature = hdr[i].first };
    // FIXPAUL get missing featureid infos

    pack->emplace_back(fid, String((char* ) data, size));
  }
}

void FeaturePackWriter::writeFeatures(const FeaturePack& pack) {
  auto header_size = sizeof(uint32_t) + sizeof(uint32_t) * 2 * pack.size();
  auto offset = header_size;

  appendUInt32(pack.size());

  for (const auto& f : pack) {
    appendUInt32(f.first.feature);
    appendUInt32(offset);
    offset += f.second.length();
  }

  for (const auto& f : pack) {
    append(f.second.c_str(), f.second.length());
  }
}

void sortFeaturePack(FeaturePack& feature_pack) {
  std::sort(feature_pack.begin(), feature_pack.end(), [] (
      const Pair<FeatureID, String>& a,
      const Pair<FeatureID, String>& b) -> bool {
    return a.first.feature < b.first.feature;
  });
}

} // namespace cm

