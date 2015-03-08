/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATUREID_H
#define _CM_FEATUREID_H
#include <fnord-base/stdtypes.h>

namespace cm {

struct FeatureID {
  uint32_t feature;
  uint16_t group;
  uint16_t flags;
};

} // namespace cm

#endif
