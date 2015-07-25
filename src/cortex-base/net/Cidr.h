// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef x0_Cidr_h
#define x0_Cidr_h

#include <cortex-base/Api.h>
#include <cortex-base/net/IPAddress.h>

namespace cortex {

/**
 * @brief CIDR network notation object.
 *
 * @see IPAddress
 */
class CORTEX_API Cidr {
 public:
  /**
   * @brief Initializes an empty cidr notation.
   *
   * e.g. 0.0.0.0/0
   */
  Cidr() : ipaddr_(), prefix_(0) {}

  /**
   * @brief Initializes this CIDR notation with given IP address and prefix.
   */
  Cidr(const char* ipaddress, size_t prefix)
      : ipaddr_(ipaddress), prefix_(prefix) {}

  /**
   * @brief Initializes this CIDR notation with given IP address and prefix.
   */
  Cidr(const IPAddress& ipaddress, size_t prefix)
      : ipaddr_(ipaddress), prefix_(prefix) {}

  /**
   * @brief Retrieves the address part of this CIDR notation.
   */
  const IPAddress& address() const { return ipaddr_; }

  /**
   * @brief Sets the address part of this CIDR notation.
   */
  bool setAddress(const std::string& text, size_t family) {
    return ipaddr_.set(text, family);
  }

  /**
   * @brief Retrieves the prefix part of this CIDR notation.
   */
  size_t prefix() const { return prefix_; }

  /**
   * @brief Sets the prefix part of this CIDR notation.
   */
  void setPrefix(size_t n) { prefix_ = n; }

  /**
   * @brief Retrieves the string-form of this network in CIDR notation.
   */
  std::string str() const;

  /**
   * @brief test whether or not given IP address is inside the network.
   *
   * @retval true it is inside this network.
   * @retval false it is not inside this network.
   */
  bool contains(const IPAddress& ipaddr) const;

  /**
   * @brief compares 2 CIDR notations for equality.
   */
  friend CORTEX_API bool operator==(const Cidr& a, const Cidr& b);

  /**
   * @brief compares 2 CIDR notations for inequality.
   */
  friend CORTEX_API bool operator!=(const Cidr& a, const Cidr& b);

 private:
  IPAddress ipaddr_;
  size_t prefix_;
};

}  // namespace cortex

namespace std {

template <>
struct hash<cortex::Cidr> : public unary_function<cortex::Cidr, size_t> {
  size_t operator()(const cortex::Cidr& v) const {
    // TODO: let it honor IPv6 better
    return *(uint32_t*)(v.address().data()) + v.prefix();
  }
};

inline std::ostream& operator<<(std::ostream& os, const cortex::Cidr& cidr) {
  os << cidr.str();
  return os;
}

}  // namespace std

#endif
