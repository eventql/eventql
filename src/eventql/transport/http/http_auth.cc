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
#include <eventql/transport/http/http_auth.h>
#include "eventql/util/http/cookies.h"
#include "eventql/util/util/Base64.h"

namespace eventql {

Status HTTPAuth::authenticateRequest(
    Session* session,
    ClientAuth* client_auth,
    const http::HTTPRequest& request) {
  HashMap<String, String> auth_data;

  if (request.hasHeader("Authorization")) {
    auto hdrval = request.getHeader("Authorization");

    {
      static const String hdrprefix = "Token ";
      if (StringUtil::beginsWith(hdrval, hdrprefix)) {
        auth_data.emplace(
            "auth_token",
            URI::urlDecode(hdrval.substr(hdrprefix.size())));
      }
    }

    {
      static const String hdrprefix = "Basic ";
      if (StringUtil::beginsWith(hdrval, hdrprefix)) {
        String basic_auth;
        util::Base64::decode(hdrval.substr(hdrprefix.size()), &basic_auth);
        auto sep_pos = basic_auth.find(":");
        if (sep_pos == String::npos) {
          auth_data.emplace("user", basic_auth);
        } else {
          auth_data.emplace("user", basic_auth.substr(0, sep_pos));
          auth_data.emplace("password", basic_auth.substr(sep_pos + 1));
        }
      }
    }
  }

  return client_auth->authenticateNonInteractive(session, auth_data);
}

}
