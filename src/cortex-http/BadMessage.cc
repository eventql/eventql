// This file is part of the "libcortex" project, http://cortex.io/
//   (c) 2009-2015 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#include <cortex-http/BadMessage.h>
#include <string>

namespace cortex {
namespace http {

std::error_category& HttpStatusCategory::get() {
  static HttpStatusCategory ec;
  return ec;
}

const char* HttpStatusCategory::name() const noexcept {
  return "http";
}

std::string HttpStatusCategory::message(int ev) const {
  return to_string(static_cast<HttpStatus>(ev));
}

BadMessage::BadMessage(HttpStatus code)
  : RuntimeError(static_cast<int>(code),
                 HttpStatusCategory::get()) {
}

BadMessage::BadMessage(HttpStatus code, const std::string& reason)
  : RuntimeError(static_cast<int>(code),
                 HttpStatusCategory::get() /*, reason*/) {
}

} // namespace http
} // namespace cortex
