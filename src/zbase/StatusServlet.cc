/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/StatusServlet.h>
#include <zbase/z1stats.h>
#include <zbase/z1.h>

using namespace stx;
namespace zbase {

void StatusServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
    http::HTTPResponse res;
  auto zs = z1stats();
  String html;

  html += R"(
    <style type="text/css">
      body, table {
        font-size: 14px;
        line-height: 20px;
        font-weight: normal;
        font-style: normal;
        font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
        padding: 0;
        margin: 0;
      }

      body {
        padding: 0 30px;
      }

      em {
        font-style: normal;
        font-weight: bold;
      }

      table td {
        padding: 6px 8px;
      }

      h1 {
        margin: 40px 0 20px 0;
      }

      h3 {
        margin: 30px 0 15px 0;
      }
    </style>
  )";

  html += StringUtil::format("<h1>Z1 $0</h1>", kVersionString);

  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Version</em></td><td align='left'>$0 &mdash; $1.$2.$3</td></tr>",
      kVersionString,
      kVersionMajor,
      kVersionMinor,
      kVersionPatch);
  html += StringUtil::format(
      "<tr><td><em>Build-ID</em></td><td align='left'>$0</td></tr>",
      kBuildID);
  html += "</table>";

  html += "<h3>Partition Map</h3>";
  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Number of Partitions</em></td><td align='right'>$0</td></tr>",
      zs->num_partitions.get());
  html += StringUtil::format(
      "<tr><td><em>Number of Partitions - Loaded</em></td><td align='right'>$0</td></tr>",
      zs->num_partitions_loaded.get());
  html += "</table>";


  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

}
