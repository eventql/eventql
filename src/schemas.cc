/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "schemas.h"
#include "JoinedSession.pb.h"
#include "IndexChangeRequest.pb.h"

using namespace cm;
using namespace fnord;

namespace cm {

void loadDefaultSchemas(msg::MessageSchemaRepository* repo) {
  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSession::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQuery::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedSearchQueryResultItem::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedCartItem::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(JoinedItemVisit::descriptor()));

  repo->registerSchema(
      msg::MessageSchema::fromProtobuf(IndexChangeRequest::descriptor()));
}

}
