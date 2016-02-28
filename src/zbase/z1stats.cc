/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/z1stats.h>

namespace zbase {

Z1Stats* z1stats() {
  static Z1Stats singleton;
  return &singleton;
}

}
