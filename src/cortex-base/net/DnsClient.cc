// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/DnsClient.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/logging.h>
#include <vector>
#include <netdb.h>

namespace cortex {

const std::vector<IPAddress>& DnsClient::ipv4(const std::string& name) {
  return lookupIP<sockaddr_in, AF_INET>(name, &ipv4_, &ipv4Mutex_);
}

const std::vector<IPAddress>& DnsClient::ipv6(const std::string& name) {
  return lookupIP<sockaddr_in6, AF_INET6>(name, &ipv6_, &ipv6Mutex_);
}

std::vector<IPAddress> DnsClient::ip(const std::string& name) {
  std::vector<IPAddress> result;
  try {
    const std::vector<IPAddress>& v4 = ipv4(name);
    for (const IPAddress& ip: v4) {
      result.push_back(ip);
    }
  } catch (...) {
  }

  try {
    const std::vector<IPAddress>& v6 = ipv6(name);
    for (const IPAddress& ip: v6) {
      result.push_back(ip);
    }
  } catch (...) {
  }

  if (result.empty())
    RAISE_STATUS(ResolveError);

  return result;
}

class GaiErrorCategory : public std::error_category {
 public:
  static std::error_category& get() {
    static GaiErrorCategory gaiErrorCategory_;
    return gaiErrorCategory_;
  }

  const char* name() const noexcept override {
    return "gai";
  }

  std::string message(int ec) const override {
    return gai_strerror(ec);
  }
};

template<typename InetType, const int AddressFamilty>
const std::vector<IPAddress>& DnsClient::lookupIP(
    const std::string& name,
    std::unordered_map<std::string, std::vector<IPAddress>>* cache,
    std::mutex* cacheMutex) {
  std::lock_guard<decltype(*cacheMutex)> _lk(*cacheMutex);
  auto i = cache->find(name);
  if (i != cache->end())
      return i->second;

  addrinfo* res = nullptr;
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;
  hints.ai_family = AddressFamilty;
  hints.ai_socktype = SOCK_STREAM; // any, actually

  int rc = getaddrinfo(name.c_str(), nullptr, &hints, &res);
  if (rc != 0)
    RAISE_CATEGORY(rc, GaiErrorCategory::get());

  std::vector<IPAddress> list;

  for (addrinfo* ri = res; ri != nullptr; ri = ri->ai_next)
    list.emplace_back(reinterpret_cast<InetType*>(ri->ai_addr));

  return (*cache)[name] = list;
}

std::vector<std::string> DnsClient::txt(const std::string& name) {
  // http://stackoverflow.com/questions/2315504/best-way-to-resolve-a-dns-txt-record-on-linux-unix-posix-bsd-type-systems
  RAISE_STATUS(NotImplementedError);
}

std::vector<std::pair<int, std::string>> DnsClient::mx(const std::string& name) {
  RAISE_STATUS(NotImplementedError);
}

void DnsClient::clearIPv4() {
  std::lock_guard<std::mutex> _lk(ipv4Mutex_);
  ipv4_.clear();
}

void DnsClient::clearIPv6() {
  std::lock_guard<std::mutex> _lk(ipv6Mutex_);
  ipv6_.clear();
}

void DnsClient::clearIP() {
  clearIPv4();
  clearIPv6();
}

}  // namespace cortex
