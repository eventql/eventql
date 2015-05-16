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

file(GLOB PROTO_FILES *.proto)

foreach(PROTO_FILE ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
  set(PROTO_SRCS ${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME}.pb.cc)
endforeach()

add_custom_target(fnord-dproc-protos ALL
    COMMAND protoc --cpp_out=. -I${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${PROTO_FILES})

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED true)

add_library(fnord-dproc OBJECT
    Application.cc
    TaskSpec.pb.cc
    LocalScheduler.cc
    ${PROTO_SRCS})


