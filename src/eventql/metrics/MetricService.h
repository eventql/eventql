/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/thread/threadpool.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/metrics/MetricQuery.h"

using namespace stx;

namespace zbase {

class MetricService {
public:

  MetricService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      zbase::TSDBService* tsdb,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
      const String& cachedir);

  void executeQuery(
      const AnalyticsSession& session,
      RefPtr<MetricQuery> query);

protected:
  ConfigDirectory* cdir_;
  AnalyticsAuth* auth_;
  zbase::TSDBService* tsdb_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  String cachedir_;
};

} // namespace zbase
