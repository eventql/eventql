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
#include <stx/random.h>
#include <stx/http/httphandler.h>

namespace zbase {

class CustomerNamespace  {
public:

  CustomerNamespace(const std::string& key);
  const std::vector<std::string>& vhosts();
  void addVHost(const std::string& hostname);

  const std::string& trackingJS();
  void loadTrackingJS(const std::string& filename);

  const std::string& key() const;

protected:
  std::string key_;
  std::string tracking_js_;
  std::vector<std::string> vhosts_;
};

} // namespace zbase
#endif
