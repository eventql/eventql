/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ITEMREF_H
#define _CM_ITEMREF_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>

namespace cm {

struct ItemRef {
  std::string set_id;
  std::string item_id;
  int position;

  bool operator==(const ItemRef& other) const;

};

} // namespace cm
#endif
