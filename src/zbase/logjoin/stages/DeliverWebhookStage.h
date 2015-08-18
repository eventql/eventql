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
#include "zbase/logjoin/SessionContext.h"

using namespace stx;

namespace zbase {

class DeliverWebhookStage {
public:

  static void process(RefPtr<SessionContext> session);

};

} // namespace zbase

