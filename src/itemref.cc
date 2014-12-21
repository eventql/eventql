/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/base/inspect.h>
#include "itemref.h"

namespace fnord {
template <>
std::string inspect(const cm::ItemRef& itemref) {
  return StringUtil::format("$0:$1", itemref.set_id, itemref.item_id);
};
}

