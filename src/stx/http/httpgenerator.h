/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __libstx_HTTPGENERATOR_H
#define __libstx_HTTPGENERATOR_H
#include <vector>
#include <string>
#include <utility>
#include <stx/io/outputstream.h>
#include <stx/http/httpresponse.h>
#include <stx/http/httprequest.h>

using stx::OutputStream;

namespace stx {
namespace http {

class HTTPGenerator {
public:
  static void generate(const HTTPRequest& req, OutputStream* os);
  static void generate(const HTTPResponse& res, OutputStream* os);
};

}
}
#endif
