/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_NET_TCPSERVER_H
#define _STX_NET_TCPSERVER_H

#include "stx/thread/taskscheduler.h"
#include "stx/net/tcpconnection.h"

namespace stx {
namespace net {

class TCPServer {
public:

  TCPServer(
      TaskScheduler* scheduler);

  void onConnection(
      std::function<void (std::unique_ptr<TCPConnection>)> callback);

  void listen(int port);

protected:
  void accept();

  int ssock_;
  TaskScheduler* scheduler_;
  std::function<void (std::unique_ptr<TCPConnection>)> on_connection_cb_;
};

}
}
#endif
