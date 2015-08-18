/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "stx/http/httprequest.h"
#include "stx/protobuf/msg.h"
#include "stx/protobuf/JSONEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "logjoin/stages/DeliverWebhookStage.h"
#include "logjoin/common.h"

using namespace stx;

namespace zbase {

void DeliverWebhookStage::process(RefPtr<SessionContext> ctx) {
  const auto& logjoin_config = ctx->customer_config->config.logjoin_config();

  for (const auto hook : logjoin_config.webhooks()) {
    stx::logDebug("logjoind", "Delivering webhook: $0", hook.target_url());

    Buffer json_buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&json_buf));

    json.beginObject();
    json.addObjectEntry("attributes");
    json.beginObject();
    size_t nattr = 0;
    for (const auto& attr : ctx->attributes()) {
      if (++nattr > 1) {
        json.addComma();
      }

      json.addObjectEntry(attr.first);
      json.addString(attr.second);
    }
    json.endObject();
    json.addComma();

    json.addObjectEntry("events");
    json.beginArray();

    size_t nevent = 0;
    for (const auto& ev : ctx->outputEvents()) {
      if (++nevent > 1) {
        json.addComma();
      }

      ev->obj.toJSON(&json);
    }

    json.endArray();
    json.endObject();

    //stx::iputs("session json: $0", json_buf.toString());

    // FIXPAUL: security risk ahead, make sure url is actually external
    http::HTTPMessage::HeaderList headers;
    headers.emplace_back(
        "X-DeepAnalytics-SessionID",
        ctx->uuid);

    auto request = http::HTTPRequest::mkPost(
        hook.target_url(),
        json_buf,
        headers);
  }
}

} // namespace zbase

