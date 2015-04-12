/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGEENCODER_H
#define _FNORD_MSG_MESSAGEENCODER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/buffer.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
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
      Vector<Pair<uint32_t, uint64_t>>* fields,
      util::BinaryMessageWriter* data);

};

} // namespace msg
} // namespace fnord

#endif
