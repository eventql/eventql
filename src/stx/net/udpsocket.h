/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_NET_UDPSOCKET_H
#define _STX_NET_UDPSOCKET_H
#include <stdlib.h>
#include "stx/buffer.h"
#include "stx/net/inetaddr.h"
#include "stx/thread/taskscheduler.h"

namespace stx {
namespace net {

class UDPSocket {
public:

  UDPSocket();
  ~UDPSocket();

  void sendTo(const Buffer& packet, const InetAddr& addr);

protected:
  int fd_;
};

}
}
#endif
