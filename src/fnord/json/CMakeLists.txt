# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

add_library(fnord-json OBJECT
    flatjsonreader.cc
    json.cc
    jsondocument.cc
    jsoninputstream.cc
    jsonoutputstream.cc
    jsonpointer.cc
    jsonrpc.cc
    jsonrpchttpadapter.cc
    jsonrpcrequest.cc
    jsonrpcresponse.cc
    jsonutil.cc)

add_executable(tests/test-json
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    json_test.cc)

target_link_libraries(tests/test-json ${CMAKE_THREAD_LIBS_INIT})
