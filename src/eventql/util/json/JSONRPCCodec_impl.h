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
namespace json {

template <typename RPCType>
void JSONRPCCodec::encodeRPCRequest(RPCType* rpc, Buffer* buffer) {
  JSONOutputStream json(
      (std::unique_ptr<OutputStream>) BufferOutputStream::fromBuffer(buffer));

  json.emplace_back(JSON_OBJECT_BEGIN);
  json.emplace_back(JSON_STRING, "jsonrpc");
  json.emplace_back(JSON_STRING, "2.0");
  json.emplace_back(JSON_STRING, "method");
  json.emplace_back(JSON_STRING, rpc->method());
  json.emplace_back(JSON_STRING, "id");
  json.emplace_back(JSON_STRING, "0"); // FIXPAUL
  json.emplace_back(JSON_STRING, "params");
  json::toJSON(rpc->args(), &json);
  json.emplace_back(JSON_OBJECT_END);

}

template <typename RPCType>
void JSONRPCCodec::decodeRPCResponse(RPCType* rpc, const Buffer& buffer) {
  auto res = parseJSON(buffer);

  auto err_iter = JSONUtil::objectLookup(res.begin(), res.end(), "error");
  if (err_iter != res.end()) {
    auto err_str_i = JSONUtil::objectLookup(err_iter, res.end(), "message");
    if (err_str_i == res.end()) {
      RAISE(kRPCError, "invalid JSONRPC response");
    } else {
      RAISE(kRPCError, err_str_i->data);
    }
  }

  auto res_iter = JSONUtil::objectLookup(res.begin(), res.end(), "result");

  ScopedPtr<typename RPCType::ResultType> decoded_res(
      new typename RPCType::ResultType(
          fromJSON<typename RPCType::ResultType>(res_iter, res.end())));

  rpc->success(std::move(decoded_res));
}


} // namespace json
