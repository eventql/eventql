/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/PipelineInfo.h"

using namespace stx;

namespace cm {

Vector<PipelineInfo> PipelineInfo::forCustomer(const CustomerConfig& cfg) {
  Vector<PipelineInfo> pipelines;

  /* session tracking */
  {
    PipelineInfo pipeline;
    pipeline.type = "inbound";
    pipeline.path = "/pipelines/session_tracking/settings";
    pipeline.name = "Session Tracking";
    pipeline.info = StringUtil::format(
        "$0 events configured",
        cfg.logjoin_config().session_event_schemas().size());
    pipeline.status = "running";
    pipelines.emplace_back(pipeline);
  }

  /* custom events */
  {
    PipelineInfo pipeline;
    pipeline.type = "inbound";
    pipeline.path = "/pipelines/custom_events/settings";
    pipeline.name = "Event Streams";
    pipeline.info = StringUtil::format(
        "$0 events configured",
        0);
    pipeline.status = "running";
    pipelines.emplace_back(pipeline);
  }

  /* logfile import */
  {
    PipelineInfo pipeline;
    pipeline.type = "inbound";
    pipeline.path = "/pipelines/logfile_import/settings";
    pipeline.name = "Logfile Import";
    pipeline.info = StringUtil::format(
        "$0 logfiles configured",
        0);
    pipeline.status = "running";
    pipelines.emplace_back(pipeline);
  }

  /* session export webhooks */
  for (const auto& webhook : cfg.logjoin_config().webhooks()) {
    PipelineInfo pipeline;
    pipeline.type = "outbound";
    pipeline.path = "/pipelines/session_tracking/webhook?id=" + webhook.id();
    pipeline.name = "Session Export Webhook";
    pipeline.info = "Target: " + webhook.target_url();
    pipeline.status = "running";
    pipelines.emplace_back(pipeline);
  }

  return pipelines;
}

} // namespace cm
