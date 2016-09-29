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
#pragma once
#include "eventql/eventql.h"
#include "eventql/server/session.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/status.h"
#include "eventql/util/return_code.h"
#include "eventql/util/http/httprequest.h"
#include "eventql/transport/native/connection.h"

namespace eventql {

class InternalAuth {
public:

  virtual ~InternalAuth() = default;

  static ReturnCode authenticateInternal(
      Session* session,
      native_transport::NativeConnection* connection,
      const std::vector<std::pair<std::string, std::string>>& auth_data);

  virtual Status verifyRequest(
      Session* session,
      const http::HTTPRequest& request) const = 0;

  virtual Status signRequest(
      Session* session,
      http::HTTPRequest* request) const = 0;

};

} // namespace eventql

