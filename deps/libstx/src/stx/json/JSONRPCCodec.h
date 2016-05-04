/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_JSONRPCCODEC_H
#define _STX_JSON_JSONRPCCODEC_H
#include <functional>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "stx/buffer.h"
#include "stx/json/json.h"

namespace stx {
namespace json {

class JSONRPCCodec {
public:

  template <typename RPCType>
  static void encodeRPCRequest(RPCType* rpc, Buffer* buffer);

  template <typename RPCType>
  static void decodeRPCResponse(RPCType* rpc, const Buffer& buffer);

};

} // namespace json
} // namespace stx

#include "JSONRPCCodec_impl.h"
#endif
