// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <functional>

namespace cortex {
namespace http {

class HttpRequest;
class HttpResponse;

typedef std::function<void(HttpRequest*, HttpResponse*)> HttpHandler;

}  // namespace http
}  // namespace cortex
