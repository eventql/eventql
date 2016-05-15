/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#ifndef _STX_JSON_JSONRPCHTTPADAPTER_H
#define _STX_JSON_JSONRPCHTTPADAPTER_H
#include <functional>
#include <stdlib.h>
#include <string>
#include <vector>
#include "eventql/util/http/httprequest.h"
#include "eventql/util/http/httpresponse.h"
#include "eventql/util/http/httpservice.h"

namespace util {
namespace json {
class JSONRPC;

class JSONRPCHTTPAdapter : public util::http::HTTPService {
public:

  static std::unique_ptr<http::HTTPService> make(JSONRPC* rpc);

  JSONRPCHTTPAdapter(
      JSONRPC* json_rpc,
      const std::string path= "/rpc");

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response) override;

protected:
  JSONRPC* json_rpc_;
  std::string path_;
};

} // namespace json
} // namespace util
#endif
