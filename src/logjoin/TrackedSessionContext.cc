/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/TrackedSessionContext.h"

using namespace stx;

namespace cm {

TrackedSessionContext::TrackedSessionContext(
    TrackedSession session) :
    uuid(session.uuid),
    customer_key(session.customer_key),
    events(session.events) {}

} // namespace cm
