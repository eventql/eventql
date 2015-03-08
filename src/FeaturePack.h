/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATUREPACK_H
#define _CM_FEATUREPACK_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include "FeatureID.h"

using namespace fnord;

namespace cm {

typedef Vector<Pair<FeatureID, String>> FeaturePack;

class FeaturePackReader : public fnord::util::BinaryMessageReader {
public:
  FeaturePackReader(const void* data, size_t size);
  void readFeatures(FeaturePack* pack);
};

class FeaturePackWriter : public fnord::util::BinaryMessageWriter {
public:
  void writeFeatures(const FeaturePack& pack);
};

void sortFeaturePack(FeaturePack& feature_pack);

} // namespace cm

#endif
