// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/net/IPAddress.h>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace cortex {

/**
 * DNS client API.
 */
class CORTEX_API DnsClient {
 public:
  /** Retrieves all IPv4 addresses for given DNS name.
   *
   * @throw if none found or an error occurred.
   */
  const std::vector<IPAddress>& ipv4(const std::string& name);

  /** Retrieves all IPv6 addresses for given DNS name.
   *
   * @throw RuntimeError if none found or an error occurred.
   */
  const std::vector<IPAddress>& ipv6(const std::string& name);

  /** Retrieves all IPv4 and IPv6 addresses for given DNS name.
   *
   * @throw RuntimeError if none found or an error occurred.
   */
  std::vector<IPAddress> ip(const std::string& name);

  /** Retrieves all TXT records for given DNS name.
   *
   * @throw RuntimeError if none found or an error occurred.
   */
  std::vector<std::string> txt(const std::string& name);

  /** Retrieves all MX records for given DNS name.
   *
   * @throw RuntimeError if none found or an error occurred.
   */
  std::vector<std::pair<int, std::string>> mx(const std::string& name);

  /**
   * Retrieves the Resource Record (DNS name) of an IP address.
   */
  std::string rr(const IPAddress& ip);

  void clearIPv4();
  void clearIPv6();
  void clearIP();
  void clearTXT();
  void clearMX();
  void clearRR();

 private:
  template<typename InetType, const int AddressFamilty>
  static const std::vector<IPAddress>& lookupIP(
      const std::string& name,
      std::unordered_map<std::string, std::vector<IPAddress>>* cache,
      std::mutex* cacheMutex);

 private:
  std::unordered_map<std::string, std::vector<IPAddress>> ipv4_;
  std::mutex ipv4Mutex_;

  std::unordered_map<std::string, std::vector<IPAddress>> ipv6_;
  std::mutex ipv6Mutex_;
};

}  // namespace cortex
