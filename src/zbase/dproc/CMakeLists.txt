# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
project(FNORD_DPROC)

include_directories(../)
include_directories(../3rdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(dproc OBJECT
    Application.cc
    DispatchService.cc
    TaskSpec.pb.cc
    TaskRef.cc
    LocalScheduler.cc
    TaskResultFuture.cc
    BlobRDD.cc
    ${PROTO_SRCS})
