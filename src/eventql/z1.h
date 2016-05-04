/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <eventql/buildconfig.h>

namespace zbase {

static const uint32_t kVersionMajor = 0;
static const uint32_t kVersionMinor = 3;
static const uint32_t kVersionPatch = 0;
static const std::string kVersionString = "v0.3.0";

#ifdef ZBASE_BUILD_ID
static const std::string kBuildID = ZBASE_BUILD_ID;
#else
static const std::string kBuildID = "unknown";
#endif

}
