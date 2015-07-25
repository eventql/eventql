/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/MessageObject.h>
#include <stx/json/json.h>

namespace stx {
namespace msg {

class JSONEncoder {
public:

  static void encode(
      const MessageObject& msg,
      const MessageSchema& schema,
      json::JSONOutputStream* json);

protected:

  static void encodeField(
      const MessageObject& msg,
      const MessageSchemaField& field,
      json::JSONOutputStream* json);

};

} // namespace msg
} // namespace stx

