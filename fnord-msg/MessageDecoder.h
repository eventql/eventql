/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGEDECODER_H
#define _FNORD_MSG_MESSAGEDECODER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/buffer.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace msg {

class MessageDecoder {
public:

  static void decode(
      const Buffer& buf,
      const MessageSchema& schema,
      MessageObject* msg);

protected:

  static void decodeObject(
      size_t idx,
      uint64_t begin,
      uint64_t end,
      const Vector<Pair<uint32_t, uint64_t>>& fields,
      const MessageSchema& schema,
      util::BinaryMessageReader* reader,
      MessageObject* msg);


};

} // namespace msg
} // namespace fnord

#endif
