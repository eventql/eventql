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
#include "stx/protobuf/MessageSchema.h"

using namespace zbase;
using namespace stx;

namespace zbase {

void loadDefaultSchemas(msg::MessageSchemaRepository* repo);

}
#endif
