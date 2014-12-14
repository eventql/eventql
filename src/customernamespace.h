/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CUSTOMERNAMESPCE_H
#define _CM_CUSTOMERNAMESPCE_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <fnord/base/random.h>
#include <fnord/net/http/httphandler.h>

namespace cm {

class CustomerNamespace  {
public:

  CustomerNamespace();
  const std::vector<std::string>& vhosts();
  void addVHost(const std::string& hostname);

  const std::string& trackingJS();
  void loadTrackingJS(const std::string& filename);

protected:
  std::string tracking_js_;
  std::vector<std::string> vhosts_;
};

} // namespace cm
#endif
