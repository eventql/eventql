/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_BROKER_BROKERCURSOR_H
#define _FNORD_BROKER_BROKERCURSOR_H

namespace stx {
namespace feeds {

class BrokerCursor {
public:

  void addServer(const URI& server, size_t offset = 0);

  void encode(Buffer* buf) const;
  void decode(const Buffer& buf);

};

}
}
#endif
