/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-sstable/SSTableServlet.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/SSTableScan.h"
#include "fnord-base/io/fileutil.h"

namespace fnord {
namespace sstable {

SSTableServlet::SSTableServlet(
    const String& base_path,
    VFS* vfs) :
    base_path_(base_path),
    vfs_(vfs) {}

void SSTableServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  if (uri.path() == base_path_ + "/scan") {
    try{
      scan(req, res, uri);
    } catch (const Exception& e) {
      res->setStatus(http::kStatusInternalServerError);
      res->addBody(StringUtil::format("error: $0", e.getMessage()));
    }
    return;
  }

  res->setStatus(fnord::http::kStatusNotFound);
  res->addBody("not found");
}

void SSTableServlet::scan(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res,
    const URI& uri) {
  fnord::URI::ParamList params = uri.queryParams();

  std::string file_path;
  if (!fnord::URI::getParam(params, "file", &file_path)) {
    res->addBody("error: missing ?file=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  sstable::SSTableReader reader(vfs_->openFile(file_path));
  if (reader.bodySize() == 0) {
    res->addBody("sstable is unfinished (body_size == 0)");
    res->setStatus(http::kStatusInternalServerError);
    return;
  }

  sstable::SSTableColumnSchema schema;
  schema.loadIndex(&reader);

  res->setStatus(fnord::http::kStatusOK);
  res->addBody("blah xx");
}

}
}
