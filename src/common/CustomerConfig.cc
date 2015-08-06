/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/MessageSchema.h"
#include "common/CustomerConfig.h"
#include "common/JoinedSession.pb.h"

using namespace stx;

namespace cm {

CustomerConfig createCustomerConfig(const String& customer) {
  CustomerConfig conf;

  conf.set_customer(customer);

  auto hook = conf.mutable_logjoin_config()->add_webhooks();
  hook->set_id("23");
  hook->set_target_url("http://localhost:8080/mywebhook");

  {
    auto ev = conf.mutable_logjoin_config()->add_session_event_schemas();
    ev->set_evtype("search_query");
    ev->set_evid(16);
    auto schema = msg::MessageSchema::fromProtobuf(cm::JoinedSearchQuery::descriptor());
    schema->setName(ev->evtype());
    ev->set_schema(schema->encode().toString());
  }

  {
    auto ev = conf.mutable_logjoin_config()->add_session_event_schemas();
    ev->set_evtype("page_view");
    ev->set_evid(25);
    auto schema = msg::MessageSchema::fromProtobuf(cm::JoinedPageView::descriptor());
    schema->setName(ev->evtype());
    ev->set_schema(schema->encode().toString());
  }

  {
    auto ev = conf.mutable_logjoin_config()->add_session_event_schemas();
    ev->set_evtype("cart_items");
    ev->set_evid(52);
    auto schema = msg::MessageSchema::fromProtobuf(cm::JoinedCartItem::descriptor());
    schema->setName(ev->evtype());
    ev->set_schema(schema->encode().toString());
  }

  conf.mutable_logjoin_config()->set_session_attributes_schema(
      msg::MessageSchema::fromProtobuf(
          cm::DefaultSessionAttributes::descriptor())->encode().toString());

  conf.mutable_logjoin_config()->set_session_schema_next_field_id(74);

  return conf;
}

} // namespace cm

