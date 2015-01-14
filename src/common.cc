/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "common.h"
#include <unistd.h>

namespace cm {

std::string cmHostname() {
  char hostname[128];
  gethostname(hostname, sizeof(hostname));
  hostname[127] = 0;
  return std::string(hostname);
}

}
