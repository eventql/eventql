/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_NET_UDPSERVER_H
#define _libstx_NET_UDPSERVER_H
#include <functional>
#include <stx/buffer.h>
#include <stx/thread/taskscheduler.h>

namespace stx {
namespace net {

class UDPServer {
public:
  UDPServer(
      TaskScheduler* server_scheduler,
      TaskScheduler* callback_scheduler);

  ~UDPServer();

  void onMessage(std::function<void (const Buffer&)> callback);
  void listen(int port);

protected:
  void messageReceived();

  TaskScheduler* server_scheduler_;
  TaskScheduler* callback_scheduler_;
  int ssock_;
  std::function<void (const stx::Buffer&)> callback_;
};


}
}
#endif
