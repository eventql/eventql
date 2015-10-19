/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/CustomerConfig.h"
#include "zbase/ConfigDirectory.h"
#include "zbase/JavaScriptContext.h"

using namespace stx;

namespace zbase {

class MapReduceService {
public:

  MapReduceService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      zbase::TSDBService* tsdb,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
      JSRuntime* js_runtime,
      const String& cachedir);

  void executeScript(
      const AnalyticsSession& session,
      const String& program_source);

  SHA1Hash mapPartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition_key,
      const String& program_source,
      const String& method_name);

protected:
  ConfigDirectory* cdir_;
  AnalyticsAuth* auth_;
  zbase::TSDBService* tsdb_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  JSRuntime* js_runtime_;
  String cachedir_;
};

} // namespace zbase
