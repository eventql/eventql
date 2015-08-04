/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <master/CustomerDirectoryMaster.h>

using namespace stx;

namespace cm {

CustomerDirectoryMaster::CustomerDirectoryMaster(
    const String& path) :
    db_path_(path) {}

void CustomerDirectoryMaster::updateCustomerConfig(CustomerConfig config) {
  RAISE(kNotYetImplementedError);
}

} // namespace cm
