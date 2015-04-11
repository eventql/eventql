/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace msg {

MessageObject::MessageObject(uint32_t id /* = 0 */) {
  new (&data_) Vector<MessageObject>();
}

//MessageObject::MessageObject(uint32_t id, uint32_t value);
//MessageObject::MessageObject(uint32_t id, String value);
//MessageObject::MessageObject(uint32_t id, bool value);

} // namespace msg
} // namespace fnord
