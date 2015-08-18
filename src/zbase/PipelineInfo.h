/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "zbase/CustomerConfig.h"
#include "zbase/TableDefinition.h"

using namespace stx;

namespace cm {

struct PipelineInfo {
  static Vector<PipelineInfo> forCustomer(const CustomerConfig& cfg);

  String type;
  String path;
  String name;
  String info;
  String status;
};

} // namespace cm
