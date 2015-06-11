/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGEPRINTER_H
#define _FNORD_MSG_MESSAGEPRINTER_H
#include <fnord-base/stdtypes.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace msg {

class MessagePrinter {
public:

  static String print(
      const MessageObject& msg,
      const MessageSchema& schema);

protected:

  static String printObject(
      size_t level,
      const MessageObject& msg,
      const MessageSchema& schema);

};

} // namespace msg
} // namespace fnord

#endif
