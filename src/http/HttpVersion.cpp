// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

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
