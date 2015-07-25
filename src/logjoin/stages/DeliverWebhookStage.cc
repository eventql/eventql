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

namespace cm {

void DeliverWebhookStage::process(RefPtr<SessionContext> ctx) {
  static auto schema = msg::MessageSchema::fromProtobuf(
      cm::JoinedSession::descriptor());

  const auto& logjoin_config = ctx->customer_config->config.logjoin_config();

  for (const auto hook : logjoin_config.webhooks()) {
    stx::logDebug("logjoind", "Delivering webhook: $0", hook.target_url());

    Buffer json_buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&json_buf));

    json.beginObject();
    json.addObjectEntry("session");

    msg::MessageObject obj;
    msg::MessageDecoder::decode(*msg::encode(ctx->session), *schema, &obj); // FIXPAUL slooow
    msg::JSONEncoder::encode(obj, *schema, &json);

    json.endObject();

    stx::iputs("session json: $0", json_buf.toString());

    // FIXPAUL: security risk ahead, make sure url is actually external
    http::HTTPMessage::HeaderList headers;
    headers.emplace_back(
        "X-DeepAnalytics-SessionID",
        ctx->session.session_id());

    auto request = http::HTTPRequest::mkPost(
        hook.target_url(),
        json_buf,
        headers);
  }
}

} // namespace cm

