/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/core/ReplicationScheme.h>
#include <eventql/core/LSMTableIndexCache.h>

#include "eventql/eventql.h"

namespace eventql {

struct ServerConfig {
  String db_path;
  RefPtr<ReplicationScheme> repl_scheme;
  RefPtr<LSMTableIndexCache> idx_cache;
};

} // namespace eventql

