# This file is part of the "libstx" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# libstx is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.

add_library(stx-json OBJECT
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

add_executable(test-json
    
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    json_test.cc)

target_link_libraries(test-json ${CMAKE_THREAD_LIBS_INIT})
