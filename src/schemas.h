/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SCHEMAS_H
#define _CM_SCHEMAS_H
#include "common.h"
#include "fnord-msg/MessageSchema.h"

using namespace cm;
using namespace fnord;

namespace cm {

void loadDefaultSchemas(msg::MessageSchemaRepository* repo);

RefPtr<msg::MessageSchema> JoinedSessionSchema();
RefPtr<msg::MessageSchema> JoinedSearchQuerySchema();
RefPtr<msg::MessageSchema> JoinedSearchQueryResultItemSchema();
RefPtr<msg::MessageSchema> JoinedItemVisitSchema();
RefPtr<msg::MessageSchema> JoinedCartItemSchema();
RefPtr<msg::MessageSchema> IndexChangeRequestSchema();
RefPtr<msg::MessageSchema> IndexChangeRequestAttributeSchema();

}
#endif
