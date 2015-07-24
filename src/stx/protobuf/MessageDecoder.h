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
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include <stx/util/binarymessagereader.h>
#include <stx/protobuf/MessageSchema.h>
#include <stx/protobuf/MessageObject.h>

namespace fnord {
namespace msg {

class MessageDecoder {
public:

  static void decode(
      const Buffer& buf,
      const MessageSchema& schema,
      MessageObject* msg);

  static void decode(
      const void* data,
      size_t size,
      const MessageSchema& schema,
      MessageObject* msg);



};

} // namespace msg
} // namespace fnord

#endif
