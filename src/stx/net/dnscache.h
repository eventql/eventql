/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_NET_DNSCACHE_H
#define _STX_NET_DNSCACHE_H
#include <mutex>
#include <string>
#include <unordered_map>
#include "stx/net/inetaddr.h"

namespace stx {
namespace net {

class DNSCache {
public:

  InetAddr resolve(const std::string& addr_str);

protected:
  std::unordered_map<std::string, InetAddr> cache_;
  std::mutex mutex_;
};

}
}

#endif
