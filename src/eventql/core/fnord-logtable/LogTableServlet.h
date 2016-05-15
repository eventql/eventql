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
#ifndef _FNORD_LOGTABLE_LOGTABLESERVLET_H
#define _FNORD_LOGTABLE_LOGTABLESERVLET_H
#include "eventql/util/http/httpservice.h"
#include "eventql/util/json/json.h"
#include <fnord-logtable/TableRepository.h>

namespace stx {
namespace logtable {

class LogTableServlet : public stx::http::HTTPService {
public:
  enum class ResponseFormat {
    JSON,
    CSV
  };

  LogTableServlet(TableRepository* tables);

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void insertRecordsBatch(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void fetchRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void fetchRecordsBatch(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void commitTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void mergeTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void gcTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void tableInfo(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  void tableSnapshot(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri);

  TableRepository* tables_;
  //ResponseFormat formatFromString(const String& format);
};

}
}
#endif
