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
#include <chartsql/svalue.h>
#include "zbase/ShopStats.pb.h"
#include "zbase/ShopStats.pb.h"

namespace zbase {

struct ShopStats {

  static void merge(const ShopKPIs& src, ShopKPIs* dst);
  static void toRow(const ShopKPIs& src, Vector<csql::SValue>* dst);
  static Vector<String> columns(const ShopKPIs& src);

  static void merge(const ShopProductKPIs& src, ShopProductKPIs* dst);
  static void toRow(const ShopProductKPIs& src, Vector<csql::SValue>* dst);
  static Vector<String> columns(const ShopProductKPIs& src);

};

} // namespace zbase
