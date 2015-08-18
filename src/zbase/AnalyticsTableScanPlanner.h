/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/UnixTime.h>
#include <dproc/Task.h>
#include <tsdb/TSDBService.h>

using namespace stx;

namespace cm {

class AnalyticsTableScanPlanner {
public:

  static List<dproc::TaskDependency> mapShards(
      const String& customer,
      const UnixTime& from,
      const UnixTime& until,
      const String& task_name,
      const Buffer& task_params,
      tsdb::TSDBService* tsdb);

  static List<dproc::TaskDependency> mapShards(
      const String& customer,
      const String& table_name,
      const String& schema_name,
      const UnixTime& from,
      const UnixTime& until,
      const String& task_name,
      const Buffer& task_params,
      tsdb::TSDBService* tsdb);


};

} // namespace cm

