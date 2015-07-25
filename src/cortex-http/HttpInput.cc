// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/HttpInput.h>

namespace cortex {
namespace http {

HttpInput::HttpInput()
    : listener_(nullptr) {
}

HttpInput::~HttpInput() {
}

void HttpInput::setListener(HttpInputListener* listener) {
  listener_ = listener;
}

} // namespace http
} // namespace cortex
