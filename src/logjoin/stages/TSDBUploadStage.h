/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/http/httpconnectionpool.h"
#include "logjoin/SessionContext.h"
#include "zbase/core/RecordEnvelope.pb.h"

using namespace stx;

namespace zbase {

class TSDBUploadStage {
public:

  static void process(
      RefPtr<SessionContext> session,
      const String& tsdb_addr,
      http::HTTPConnectionPool* http);

private:

  static void serializeSession(
      RefPtr<SessionContext> session,
      zbase::RecordEnvelopeList* records);

  static void serializeEvent(
      RefPtr<SessionContext> ctx,
      RefPtr<OutputEvent> ev,
      zbase::RecordEnvelopeList* records);

};

} // namespace zbase

