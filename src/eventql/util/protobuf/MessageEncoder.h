/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MSG_MESSAGEENCODER_H
#define _STX_MSG_MESSAGEENCODER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>

namespace stx {
namespace msg {

class MessageEncoder {
public:

  static void encode(
      const MessageObject& msg,
      const MessageSchema& schema,
      Buffer* buf);

protected:

  static void encodeObject(
      const MessageObject& msg,
      const MessageSchema& schema,
      util::BinaryMessageWriter* data);

};

} // namespace msg
} // namespace stx

#endif
