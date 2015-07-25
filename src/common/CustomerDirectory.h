/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <common/CustomerConfig.pb.h>

using namespace stx;

namespace cm {

class CustomerDirectory {
public:

  CustomerConfig customerConfigFor(const String& customer_key);

  LogJoinConfig logjoinConfigFor(const String& customer_key);

  void updateCustomerConfig(CustomerConfig config);

protected:
  std::mutex mutex_;
  HashMap<String, CustomerConfig> customers_;
};

} // namespace cm
