/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/http/VFSFileServlet.h"
#include "stx/io/fileutil.h"

namespace stx {
namespace http {

VFSFileServlet::VFSFileServlet(
    const String& base_path,
    VFS* vfs) :
    base_path_(base_path),
    vfs_(vfs) {}

void VFSFileServlet::handleHTTPRequest(
    stx::http::HTTPRequest* req,
    stx::http::HTTPResponse* res) {
  URI uri(req->uri());
  stx::URI::ParamList params = uri.queryParams();

  res->addHeader("Access-Control-Allow-Origin", "*");

  std::string file_path;
  if (!stx::URI::getParam(params, "file", &file_path)) {
    res->addBody("error: missing ?file=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  if (uri.path() == base_path_ + "/get") {
    auto file = vfs_->openFile(file_path);
    res->setStatus(stx::http::kStatusOK);
    res->addHeader("Content-Type", contentTypeFromFilename(file_path));
    res->addBody(file->data(), file->size());
    return;
  }

  if (uri.path() == base_path_ + "/size") {
    auto file = vfs_->openFile(file_path);
    res->setStatus(stx::http::kStatusOK);
    res->addBody(StringUtil::toString(file->size()));
    return;
  }

  res->setStatus(stx::http::kStatusNotFound);
  res->addBody("not found");
}

String VFSFileServlet::contentTypeFromFilename(const String& filename) const {
  if (StringUtil::endsWith(filename, ".csv")) {
    return "text/csv; charset=utf-8";
  }

  if (StringUtil::endsWith(filename, ".html")) {
    return "text/html; charset=utf-8";
  }

  if (StringUtil::endsWith(filename, ".json")) {
    return "application/json; charset=utf-8";
  }

  if (StringUtil::endsWith(filename, ".txt")) {
    return "text/plain; charset=utf-8";
  }

  return "application/octet-stream";
}

}
}
