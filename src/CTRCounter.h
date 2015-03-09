/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRCOUNTER_H
#define _CM_CTRCOUNTER_H
#include "fnord-base/stdtypes.h"
#include "fnord-base/option.h"

using namespace fnord;

namespace cm {

struct CTRCounter {
  static void write(const CTRCounter& counter, Buffer* buf);
  static CTRCounter load(const Buffer& buf);

  CTRCounter();
  void merge(const CTRCounter& other);

  uint64_t num_views;
  uint64_t num_clicked;
  uint64_t num_clicks;
};


}
#endif
