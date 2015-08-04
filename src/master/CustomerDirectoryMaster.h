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
#include <stx/SHA1.h>
#include <common/CustomerConfig.h>
#include <common/TableDefinition.h>

using namespace stx;

namespace cm {

class CustomerDirectoryMaster {
public:

  CustomerDirectoryMaster(const String& path);

  void updateCustomerConfig(CustomerConfig config);

protected:

  void loadHeads();

  mutable std::mutex mutex_;
  String db_path_;
  HashMap<String, uint64_t> heads_;
};

} // namespace cm
