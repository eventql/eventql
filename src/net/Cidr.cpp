// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/Cidr.h>
#include <xzero/net/IPAddress.h>

namespace xzero {

std::string Cidr::str() const {
  char result[INET6_ADDRSTRLEN + 32];

  inet_ntop(ipaddr_.family(), ipaddr_.data(), result, sizeof(result));

  size_t n = strlen(result);
  snprintf(result + n, sizeof(result) - n, "/%zu", prefix_);

  return result;
}

bool Cidr::contains(const IPAddress& ipaddr) const {
  if (ipaddr.family() != address().family()) return false;

  // IPv4
  if (ipaddr.family() == IPAddress::V4) {
    uint32_t ip = *(uint32_t*)ipaddr.data();
    uint32_t subnet = *(uint32_t*)address().data();
    uint32_t match = ip & (0xFFFFFFFF >> (32 - prefix()));

    return match == subnet;
  }

  // IPv6
  int bits = prefix();
  const uint32_t* words = (const uint32_t*)address().data();
  const uint32_t* input = (const uint32_t*)ipaddr.data();
  while (bits >= 32) {
    uint32_t match = *words & 0xFFFFFFFF;
    if (match != *input) return false;

    words++;
    input++;
    bits -= 32;
  }

  uint32_t match = *words & 0xFFFFFFFF >> (32 - bits);
  if (match != *input) return false;

  return true;
}

bool operator==(const Cidr& a, const Cidr& b) {
  if (&a == &b) return true;

  if (a.address().family() != b.address().family()) return false;

  switch (a.address().family()) {
    case AF_INET:
    case AF_INET6:
      return memcmp(a.address().data(), b.address().data(),
                    a.address().size()) == 0 &&
             a.prefix_ == b.prefix_;
    default:
      return false;
  }

  return false;
}

bool operator!=(const Cidr& a, const Cidr& b) { return !(a == b); }

}  // namespace xzero
