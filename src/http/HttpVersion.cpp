#include <xzero/http/HttpVersion.h>
#include <stdexcept>

namespace xzero {

#define SRET(slit) { static std::string val(slit); return val; }

const std::string& to_string(HttpVersion version) {
  switch (version) {
    case HttpVersion::VERSION_0_9: SRET("0.9");
    case HttpVersion::VERSION_1_0: SRET("1.0");
    case HttpVersion::VERSION_1_1: SRET("1.1");
    case HttpVersion::VERSION_2_0: SRET("2.0");
    case HttpVersion::UNKNOWN:
    default:
      throw std::runtime_error("Invalid Argument. Invalid HttpVersion value.");
  }
}

} // namespace xzero
