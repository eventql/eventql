/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/auth/client_auth_legacy.h"
#include "eventql/util/protobuf/msg.h"

namespace eventql {

LegacyClientAuth::LegacyClientAuth(
    const String& secret) :
    cookie_coder_(secret) {}

Status LegacyClientAuth::authenticateNonInteractive(
    Session* session,
    HashMap<String, String> auth_data) {
  const auto& auth_token_str = auth_data["auth_token"];
  if (auth_token_str.empty()) {
    return Status(eRuntimeError, "missing auth token");
  }

  auto auth_token = cookie_coder_.decodeAndVerify(auth_token_str);
  if (auth_token.isEmpty()) {
    return Status(eRuntimeError, "invalid auth token");
  }

  auto token_data = msg::decode<LegacyAuthTokenData>(auth_token.get().data());
  session->setEffectiveNamespace(token_data.db_namespace());
  session->setUserID(token_data.userid());

  return Status::success();
}

Status LegacyClientAuth::changeNamespace(
    Session* session,
    const String& ns) {
  if (session->isInternal()) {
    session->setEffectiveNamespace(ns);
    session->setDisplayNamespace(ns);
    return Status::success();
  } else if (ns == session->getEffectiveNamespace()) {
    return Status::success();
  } else {
    return Status(eRuntimeError, "access denied");
  }
}

} // namespace eventql

