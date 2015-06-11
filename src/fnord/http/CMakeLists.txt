# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

add_library(fnord-http OBJECT
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
    HTTPSSEStream.cc)
    #streaminghttpservice_example/HttpServlet.cc
    #streaminghttpservice_example/ReadStreamServlet.cc
    #streaminghttpservice_example/SSEStreamServlet.cc
    #streaminghttpservice_example/WriteStreamServlet.cc)

add_executable(tests/test-http
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    http_test.cc)

target_link_libraries(tests/test-http ${CMAKE_THREAD_LIBS_INIT})

