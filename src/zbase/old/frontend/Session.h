/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINSERVICE_H
#define _CM_LOGJOINSERVICE_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <zbase/docdb/ItemRef.h>
#include "trackedquery.h"

namespace zbase {

class Session {
  Session();

protected:
  std::unordered_map<std::string, TrackedQuery> queries_;
};

} // namespace zbase
#endif
