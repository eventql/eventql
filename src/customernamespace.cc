/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/io/fileutil.h>
#include "customernamespace.h"

namespace cm {

CustomerNamespace::CustomerNamespace() {
}

const std::vector<std::string>& CustomerNamespace::vhosts() {
  return vhosts_;
}

void CustomerNamespace::addVHost(const std::string& hostname) {
  vhosts_.emplace_back(hostname);
}

const std::string& CustomerNamespace::trackingJS() {
  return tracking_js_;
}

void CustomerNamespace::loadTrackingJS(const std::string& filename) {
  tracking_js_ = fnord::io::FileUtil::read(filename);
}

} // namespace cm
