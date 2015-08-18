/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/autoref.h"
#include "zbase/CustomerConfig.pb.h"

using namespace stx;

namespace cm {

struct CustomerConfigRef : public RefCounted {
  CustomerConfigRef(CustomerConfig _config) : config(_config) {}
  CustomerConfig config;
};

CustomerConfig createCustomerConfig(const String& customer);

} // namespace cm

