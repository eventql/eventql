/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_HTTP_VFSFILESERVLET_H
#define _STX_HTTP_VFSFILESERVLET_H
#include "stx/VFS.h"
#include "stx/http/httpservice.h"

namespace stx {
namespace http {

class VFSFileServlet : public stx::http::HTTPService {
public:

  VFSFileServlet(const String& base_path, VFS* vfs);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res);

protected:
  String contentTypeFromFilename(const String& filename) const;

  String base_path_;
  VFS* vfs_;
};

}
}
#endif
