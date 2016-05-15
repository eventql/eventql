/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/infra/sstable/SSTableServlet.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableScan.h"
#include "eventql/util/io/fileutil.h"


namespace sstable {

SSTableServlet::SSTableServlet(
    const String& base_path,
    VFS* vfs) :
    base_path_(base_path),
    vfs_(vfs) {}

void SSTableServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
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

  res->setStatus(http::kStatusNotFound);
  res->addBody("not found");
}

void SSTableServlet::scan(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    const URI& uri) {
  URI::ParamList params = uri.queryParams();

  auto format = ResponseFormat::CSV;
  std::string format_param;
  if (URI::getParam(params, "format", &format_param)) {
    format = formatFromString(format_param);
  }

  std::string file_path;
  if (!URI::getParam(params, "file", &file_path)) {
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

  sstable::SSTableScan sstable_scan(&schema);

  String limit_str;
  if (URI::getParam(params, "limit", &limit_str)) {
    sstable_scan.setLimit(std::stoul(limit_str));
  }

  String key_prefix_str;
  if (URI::getParam(params, "key_prefix", &key_prefix_str)) {
    sstable_scan.setKeyPrefix(key_prefix_str);
  }

  String key_regex_str;
  if (URI::getParam(params, "key_regex", &key_regex_str)) {
    sstable_scan.setKeyFilterRegex(key_regex_str);
  }

  Set<String> key_match_set;
  for (const auto& p : params) {
    if (p.first == "key_match") {
      key_match_set.emplace(p.second);
    }
  }
  if (key_match_set.size() > 0) {
    sstable_scan.setKeyExactMatchFilter(key_match_set);
  }

  String offset_str;
  if (URI::getParam(params, "offset", &offset_str)) {
    sstable_scan.setOffset(std::stoul(offset_str));
  }

  String order_by;
  String order_fn = "STRASC";
  if (URI::getParam(params, "order_by", &order_by)) {
    URI::getParam(params, "order_fn", &order_fn);
    sstable_scan.setOrderBy(order_by, order_fn);
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  auto headers = sstable_scan.columnNames();

  switch (format) {
    case ResponseFormat::CSV:
      buf.append(StringUtil::join(headers, ";"));
      buf.append("\n");
      break;
    case ResponseFormat::JSON:
      json.beginArray();
      break;
  }

  auto cursor = reader.getCursor();
  int n = 0;
  sstable_scan.execute(
      cursor.get(),
      [&buf, format, &n, &json, &headers] (const Vector<String> row) {
    switch (format) {
      case ResponseFormat::CSV:
        buf.append(StringUtil::join(row, ";"));
        buf.append("\n");
        break;
      case ResponseFormat::JSON:
        if (n++ > 0) json.addComma();
        json.beginObject();
        for (int i = 0; i < row.size(); ++i) {
          if (i > 0) json.addComma();
          json.addString(headers[i]);
          json.addColon();
          json.addString(row[i]);
        }
        json.endObject();
        break;
    }
  });

  switch (format) {
    case ResponseFormat::CSV:
      res->addHeader("Content-Type", "text/csv; charset=utf-8");
      break;
    case ResponseFormat::JSON:
      json.endArray();
      res->addHeader("Content-Type", "application/json; charset=utf-8");
      break;
  }

  res->setStatus(http::kStatusOK);
  res->addBody(buf);
}

SSTableServlet::ResponseFormat SSTableServlet::formatFromString(
    const String& format) {
  if (format == "csv") {
    return ResponseFormat::CSV;
  }

  if (format == "json") {
    return ResponseFormat::JSON;
  }

  RAISEF(kIllegalArgumentError, "invalid format: $0", format);
}

}
