/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/exception.h>
#include <common/CustomerDirectory.h>

using namespace stx;

namespace cm {

RefPtr<CustomerConfigRef> CustomerDirectory::configFor(
    const String& customer_key) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = customers_.find(customer_key);
  if (iter == customers_.end()) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  return iter->second;
}

void CustomerDirectory::updateCustomerConfig(CustomerConfig config) {
  std::unique_lock<std::mutex> lk(mutex_);
  customers_.emplace(config.customer(), new CustomerConfigRef(config));
}

} // namespace cm
