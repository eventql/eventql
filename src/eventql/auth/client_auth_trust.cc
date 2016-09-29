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
#include "eventql/auth/client_auth_trust.h"

namespace eventql {

Status TrustClientAuth::authenticateNonInteractive(
    Session* session,
    HashMap<String, String> auth_data) {
  const auto& user_id = auth_data["user"];
  if (user_id.empty()) {
    session->setUserID("nobody");
    return Status::success();
  } else {
    session->setUserID(user_id);
    return Status::success();
  }
}

Status TrustClientAuth::changeNamespace(
    Session* session,
    const String& ns) {
  session->setEffectiveNamespace(ns);
  session->setDisplayNamespace(ns);
  return Status::success();
}

} // namespace eventql

