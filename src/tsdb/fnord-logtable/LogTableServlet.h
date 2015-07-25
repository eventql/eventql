/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_LOGTABLESERVLET_H
#define _FNORD_LOGTABLE_LOGTABLESERVLET_H
#include "stx/http/httpservice.h"
#include "stx/json/json.h"
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
