# Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
# Authors:
#   - Paul Asmuth <paul@zscale.io>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License ("the license") as
# published by the Free Software Foundation, either version 3 of the License,
# or any later version.
#
# In accordance with Section 7(e) of the license, the licensing of the Program
# under the license does not imply a trademark license. Therefore any rights,
# title and interest in our trademarks remain entirely with us.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the license for more details.
#
# You can be released from the requirements of the license by purchasing a
# commercial license. Buying such a license is mandatory as soon as you develop
# commercial activities involving this program without disclosing the source
# code of your own applications

add_library(stx-http STATIC
    cookies.cc
    httpclient.cc
    httpclientconnection.cc
    httpconnectionpool.cc
    HTTPFileDownload.cc
    httpgenerator.cc
    httpmessage.cc
    httpparser.cc
    httprequest.cc
    httpresponse.cc
    httpresponsefuture.cc
    httprouter.cc
    HTTPRequestStream.cc
    HTTPResponseStream.cc
    httpserver.cc
    httpserverconnection.cc
    httpservice.cc
    statshttpservlet.cc
    VFSFileServlet.cc
    HTTPSSEParser.cc
    HTTPSSEStream.cc
    HTTPSSEResponseHandler.cc)
    #streaminghttpservice_example/HttpServlet.cc
    #streaminghttpservice_example/ReadStreamServlet.cc
    #streaminghttpservice_example/SSEStreamServlet.cc
    #streaminghttpservice_example/WriteStreamServlet.cc)

if(STX_BUILD_UNIT_TESTS)
  add_executable(test-http http_test.cc)
  target_link_libraries(test-http stx-http stx-base stx-json)
endif()
