/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/inspect.h>
#include "FeatureID.h"

namespace fnord {

template <>
String inspect(const cm::FeatureID& fid) {
  return StringUtil::format("feature($0,$1)", fid.feature, fid.group);
}

}
