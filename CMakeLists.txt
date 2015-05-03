# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
project(FNORD_TSDB)

include_directories(../)
include_directories(../3rdparty)

add_library(fnord-tsdb OBJECT
    TSDBNode.cc
    TSDBServlet.cc
    RecordSet.cc)

add_executable(tests/test-recordset
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-http>
    RecordSet_test.cc)

